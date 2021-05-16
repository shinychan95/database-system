/******************************************************************************/
/*                                                                            */
/*    ODYSSEUS/EduCOSMOS Educational-Purpose Object Storage System            */
/*                                                                            */
/*    Developed by Professor Kyu-Young Whang et al.                           */
/*                                                                            */
/*    Database and Multimedia Laboratory                                      */
/*                                                                            */
/*    Computer Science Department and                                         */
/*    Advanced Information Technology Research Center (AITrc)                 */
/*    Korea Advanced Institute of Science and Technology (KAIST)              */
/*                                                                            */
/*    e-mail: kywhang@cs.kaist.ac.kr                                          */
/*    phone: +82-42-350-7722                                                  */
/*    fax: +82-42-350-8380                                                    */
/*                                                                            */
/*    Copyright (c) 1995-2013 by Kyu-Young Whang                              */
/*                                                                            */
/*    All rights reserved. No part of this software may be reproduced,        */
/*    stored in a retrieval system, or transmitted, in any form or by any     */
/*    means, electronic, mechanical, photocopying, recording, or otherwise,   */
/*    without prior written permission of the copyright owner.                */
/*                                                                            */
/******************************************************************************/
/*
 * Module: EduBtM_Fetch.c
 *
 * Description :
 *  Find the first object satisfying the given condition.
 *  If there is no such object, then return with 'flag' field of cursor set
 *  to CURSOR_EOS. If there is an object satisfying the condition, then cursor
 *  points to the object position in the B+ tree and the object identifier
 *  is returned via 'cursor' parameter.
 *  The condition is given with a key value and a comparison operator;
 *  the comparison operator is one among SM_BOF, SM_EOF, SM_EQ, SM_LT, SM_LE, SM_GT, SM_GE.
 *
 * Exports:
 *  Four EduBtM_Fetch(PageID*, KeyDesc*, KeyValue*, Four, KeyValue*, Four, BtreeCursor*)
 */


#include <string.h>
#include "EduBtM_common.h"
#include "BfM.h"
#include "EduBtM_Internal.h"


/*@ Internal Function Prototypes */
Four edubtm_Fetch(PageID*, KeyDesc*, KeyValue*, Four, KeyValue*, Four, BtreeCursor*);



/*@================================
 * EduBtM_Fetch()
 *================================*/
/*
 * Function: Four EduBtM_Fetch(PageID*, KeyDesc*, KeyVlaue*, Four, KeyValue*, Four, BtreeCursor*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Find the first object satisfying the given condition. See above for detail.
 *
 * Returns:
 *  error code
 *    eBADPARAMETER_BTM
 *    some errors caused by function calls
 *
 * Side effects:
 *  cursor  : The found ObjectID and its position in the Btree Leaf
 *            (it may indicate a ObjectID in an  overflow page).
 * 
 * 한글 설명:
 *  B+ tree 색인에서 검색 조건을 만족하는 첫 번째 object를 검색하고, 검색된 object를 가리키는 cursor를 반환함
 * 
 * 관련 함수:
 *  - edubtm_Fetch()
 *  - edubtm_FirstObject()
 *  - edubtm_LastObject()
 */
Four EduBtM_Fetch(
    PageID   *root,		/* IN The current root of the subtree */
    KeyDesc  *kdesc,		/* IN Btree key descriptor */
    KeyValue *startKval,	/* IN key value of start condition */
    Four     startCompOp,	/* IN comparison operator of start condition */
    KeyValue *stopKval,		/* IN key value of stop condition */
    Four     stopCompOp,	/* IN comparison operator of stop condition */
    BtreeCursor *cursor)	/* OUT Btree Cursor */
{
    int i;
    Four e;		   /* error number */

    
    if (root == NULL) ERR(eBADPARAMETER_BTM);

    /* Error check whether using not supported functionality by EduBtM */
    for (i = 0; i < kdesc->nparts; i++) {
        if (kdesc->kpart[i].type != SM_INT && kdesc->kpart[i].type != SM_VARSTRING) {
            ERR(eNOTSUPPORTED_EDUBTM);
        }
    }

    // 파라미터로 주어진 startCompOp가 SM_BOF일 경우,
    if (startCompOp == SM_BOF) {
        e = edubtm_FirstObject(root, kdesc, stopKval, stopCompOp, cursor);
    }   
    // 파라미터로 주어진 startCompOp가 SM_EOF일 경우,
    else if (startCompOp == SM_EOF) {
        e = edubtm_LastObject(root, kdesc, stopKval, stopCompOp, cursor);
    }
    // 이외의 경우,
    else {
        e = edubtm_Fetch(root, kdesc, startKval, startCompOp, stopKval, stopCompOp, cursor);
    }
    if (e < eNOERROR) ERR(e);

    return(eNOERROR);

} /* EduBtM_Fetch() */



/*@================================
 * edubtm_Fetch()
 *================================*/
/*
 * Function: Four edubtm_Fetch(PageID*, KeyDesc*, KeyVlaue*, Four, KeyValue*, Four, BtreeCursor*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Find the first object satisfying the given condition.
 *  This function handles only the following conditions:
 *  SM_EQ, SM_LT, SM_LE, SM_GT, SM_GE.
 *
 * Returns:
 *  Error code *   
 *    eBADCOMPOP_BTM
 *    eBADBTREEPAGE_BTM
 *    some errors caused by function calls
 * 
 * 한글 설명:
 *  파라미터로 주어진 page를 root page로 하는 B+ tree 색인에서 검색 조건을 만족하는 
 *  첫 번째 <object의 key, object ID> pair가 저장된 leaf index entry를 검색하고, 
 *  검색된 leaf index entry를 가리키는 cursor를 반환함. 첫 번째 object는, 
 *  검색 조건을 만족하는 object들 중 검색 시작 key 값과 가장 가까운 key 값을 가지는 object를 의미함
 * 
 * 관련 함수:
 *  - edubtm_BinarySearchLeaf(), 
 *  - edubtm_BinarySearchInternal(), 
 *  - edubtm_KeyCompare()
 *    - 파라미터로 주어진 두 key 값의 대소를 비교하고, 비교 결과를 반환함. 
 *    - Variable length string의 경우 사전식 순서를 이용하여 비교함
 *    - 두 key 값이 같은 경우, EQUAL을 반환함
 *    - 첫 번째 key 값이 큰 경우, GREATER를 반환함
 *    - 첫 번째 key 값이 작은 경우, LESS를 반환함
 * 
 *  - BfM_GetTrain(), 
 *  - BfM_FreeTrain()
 */
Four edubtm_Fetch(
    PageID              *root,          /* IN The current root of the subtree */
    KeyDesc             *kdesc,         /* IN Btree key descriptor */
    KeyValue            *startKval,     /* IN key value of start condition */
    Four                startCompOp,    /* IN comparison operator of start condition */
    KeyValue            *stopKval,      /* IN key value of stop condition */
    Four                stopCompOp,     /* IN comparison operator of stop condition */
    BtreeCursor         *cursor)        /* OUT Btree Cursor */
{
    Four                e;              /* error number */
    Four                cmp;            /* result of comparison */
    Two                 idx;            /* index */
    PageID              child;          /* child page when the root is an internla page */
    Two                 alignedKlen;    /* aligned size of the key length */
    BtreePage           *apage;         /* a Page Pointer to the given root */
    BtreeOverflow       *opage;         /* a page pointer if it necessary to access an overflow page */
    Boolean             found;          /* search result */
    PageID              *leafPid;       /* leaf page pointed by the cursor */
    Two                 slotNo;         /* slot pointed by the slot */
    PageID              ovPid;          /* PageID of the overflow page */
    PageNo              ovPageNo;       /* PageNo of the overflow page */
    PageID              prevPid;        /* PageID of the previous page */
    PageID              nextPid;        /* PageID of the next page */
    ObjectID            *oidArray;      /* array of the ObjectIDs */
    Two                 iEntryOffset;   /* starting offset of an internal entry */
    btm_InternalEntry   *iEntry;        /* an internal entry */
    Two                 lEntryOffset;   /* starting offset of a leaf entry */
    btm_LeafEntry       *lEntry;        /* a leaf entry */


    /* Error check whether using not supported functionality by EduBtM */
    int i;
    for (i = 0; i < kdesc->nparts; i++) {
        if (kdesc->kpart[i].type != SM_INT && kdesc->kpart[i].type != SM_VARSTRING) {
            ERR(eNOTSUPPORTED_EDUBTM);
        }
    }

    e = BfM_GetTrain(root, (char**)&apage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    // 파라미터로 주어진 root page가 internal page인 경우,
    if (apage->any.hdr.type & INTERNAL) {
        // 다음으로 방문할 자식 page를 결정함

        // startCompOp가 GREATEST인 경우,
        if (startCompOp & SM_EOF) {
            idx = apage->bi.hdr.nSlots - 1;

            child.volNo = root->volNo;
            iEntry = (btm_InternalEntry*)&apage->bi.data[apage->bi.slot[-idx]];
            child.pageNo = iEntry->spid;
        }
        // startCompOp가 LEAST인 경우,
        else if (startCompOp & SM_BOF) {
            child.volNo = root->volNo;
            child.pageNo = apage->bi.hdr.p0;
        }
        // 그 외,
        else {
            edubtm_BinarySearchInternal(apage, kdesc, startKval, &idx);

            child.volNo = root->volNo;
            if (idx == -1) {
                child.pageNo = apage->bi.hdr.p0;
            }
            else {
                iEntry = (btm_InternalEntry*)&apage->bi.data[apage->bi.slot[-idx]];
                child.pageNo = iEntry->spid;
            }
        }
        
        e = edubtm_Fetch(&child, kdesc, startKval, startCompOp, stopKval, stopCompOp, cursor);
        if (e < eNOERROR) ERRB1(e, root, PAGE_BUF);
    }
    // 파라미터로 주어진 root page가 leaf page인 경우,
    else if (apage->any.hdr.type & LEAF) {
        // 검색 조건을 만족하는 첫 번째 <object의 key, object ID> pair가 저장된 index entry를 검색함

        // 목표: 검색 시작 key 값과 가장 가까운 key 값을 찾는다.

        // SM_EQ: 검색 시작/종료 key 값과 같은           (0x1 = 1 = 001)
        // SM_LT: 검색 시작/종료 key 값보다 작은         (0x2 = 2 = 010)
        // SM_LE: 검색 시작/종료 key 값보다 작거나 같은   (0x3 = 3 = 011)
        // SM_GT: 검색 시작/종료 key 값보다 큰           (0x4 = 4 = 100)
        // SM_GE: 검색 시작/종료 key 값보다 크거나 같은   (0x5 = 5 = 101)
        // SM_EOF: GREATEST                            (0x10 = 16 = 10000)
        // SM_BOF: LEAST                                (0x20 = 32 = 20000)

        // Cursor를 반환한다. (Cursor에는 flag, oid, key, leaf, slotNo)

        // Output Example -> Key: 199, OID: (1000, 777, 199, 199)

        // startCompOp가 GREATEST인 경우,
        if (startCompOp & SM_EOF) {
            cursor->flag = CURSOR_ON;
            slotNo = apage->bl.hdr.nSlots - 1;
            leafPid = root;

            lEntry = (btm_LeafEntry*)&apage->bl.data[apage->bl.slot[-slotNo]];
            
            cursor->oid = *(ObjectID*)&lEntry->kval[ALIGNED_LENGTH(lEntry->klen)];
            cursor->key.len = lEntry->klen;
	        memcpy(&(cursor->key.val[0]), &lEntry->kval, lEntry->klen);
            cursor->leaf = *leafPid;
            cursor->slotNo = slotNo;
        }
        // startCompOp가 LEAST인 경우,
        else if (startCompOp & SM_BOF) {
            cursor->flag = CURSOR_ON;
            slotNo = 0;
            leafPid = root;

            lEntry = (btm_LeafEntry*)&apage->bl.data[apage->bl.slot[-slotNo]];
            
            cursor->oid = *(ObjectID*)&lEntry->kval[ALIGNED_LENGTH(lEntry->klen)];
            cursor->key.len = lEntry->klen;
	        memcpy(&(cursor->key.val[0]), &lEntry->kval, lEntry->klen);
            cursor->leaf = *leafPid;
            cursor->slotNo = slotNo;
        }
        // 그 외,
        else {
            found = edubtm_BinarySearchLeaf(apage, kdesc, startKval, &idx);

            // 같다는 조건 있으면, 발견한 slot no로 반환
            if ((startCompOp & SM_EQ) && found) {
                cursor->flag = CURSOR_ON;
                slotNo = idx;
                leafPid = root;

                lEntry = (btm_LeafEntry*)&apage->bl.data[apage->bl.slot[-slotNo]];
            
                cursor->oid = *(ObjectID*)&lEntry->kval[ALIGNED_LENGTH(lEntry->klen)];
                cursor->key.len = lEntry->klen;
                memcpy(&(cursor->key.val[0]), &lEntry->kval, lEntry->klen);
                cursor->leaf = *leafPid;
                cursor->slotNo = slotNo;
            }
            
            else if (startCompOp & SM_LT) {
                // 주어진 key 값보다 작은 entry가 없을 경우,
                if (idx == -1) {
                    if (apage->bl.hdr.prevPage == NIL) {
                        cursor->flag = CURSOR_EOS;
                    }
                    else {
                        MAKE_PAGEID(prevPid, root->volNo, apage->bl.hdr.prevPage);
                        
                        e = BfM_GetTrain(&prevPid, (char**)&apage, PAGE_BUF);
                        if (e < eNOERROR) ERRB1(e, root, PAGE_BUF);
                        
                        cursor->flag = CURSOR_ON;
                        slotNo = apage->bl.hdr.nSlots - 1;
                        leafPid = &prevPid;

                        lEntry = (btm_LeafEntry*)&apage->bl.data[apage->bl.slot[-slotNo]];
            
                        cursor->oid = *(ObjectID*)&lEntry->kval[ALIGNED_LENGTH(lEntry->klen)];
                        cursor->key.len = lEntry->klen;
                        memcpy(&(cursor->key.val[0]), &lEntry->kval, lEntry->klen);
                        cursor->leaf = *leafPid;
                        cursor->slotNo = slotNo;
                    }
                }
                else {
                    cursor->flag = CURSOR_ON;
                    slotNo = found ? idx - 1: idx;
                    leafPid = root;

                    lEntry = (btm_LeafEntry*)&apage->bl.data[apage->bl.slot[-slotNo]];
            
                    cursor->oid = *(ObjectID*)&lEntry->kval[ALIGNED_LENGTH(lEntry->klen)];
                    cursor->key.len = lEntry->klen;
                    memcpy(&(cursor->key.val[0]), &lEntry->kval, lEntry->klen);
                    cursor->leaf = *leafPid;
                    cursor->slotNo = slotNo;
                }
            }
            else if (startCompOp & SM_GT) {
                if (idx == apage->bl.hdr.nSlots - 1) {
                    if (apage->bl.hdr.nextPage == NIL) {
                        cursor->flag = CURSOR_EOS;
                    }
                    else {
                        MAKE_PAGEID(nextPid, root->volNo, apage->bl.hdr.nextPage);
                        
                        e = BfM_GetTrain(&prevPid, (char**)&apage, PAGE_BUF);
                        if (e < eNOERROR) ERRB1(e, root, PAGE_BUF);
                        
                        cursor->flag = CURSOR_ON;
                        slotNo = 0;
                        leafPid = &nextPid;

                        lEntry = (btm_LeafEntry*)&apage->bl.data[apage->bl.slot[-slotNo]];
            
                        cursor->oid = *(ObjectID*)&lEntry->kval[ALIGNED_LENGTH(lEntry->klen)];
                        cursor->key.len = lEntry->klen;
                        memcpy(&(cursor->key.val[0]), &lEntry->kval, lEntry->klen);
                        cursor->leaf = *leafPid;
                        cursor->slotNo = slotNo;
                    }
                }
                else {
                    cursor->flag = CURSOR_ON;
                    slotNo = idx + 1;
                    leafPid = root;

                    lEntry = (btm_LeafEntry*)&apage->bl.data[apage->bl.slot[-slotNo]];
            
                    cursor->oid = *(ObjectID*)&lEntry->kval[ALIGNED_LENGTH(lEntry->klen)];
                    cursor->key.len = lEntry->klen;
                    memcpy(&(cursor->key.val[0]), &lEntry->kval, lEntry->klen);
                    cursor->leaf = *leafPid;
                    cursor->slotNo = slotNo;
                }
            }
            else {
                cursor->flag = CURSOR_EOS;
            }
        }
        
        if (cursor->flag == CURSOR_ON) {
            // CURSOR_ON 이더라도, stopCompOp 및 stopKval로 인해 결과가 없는 경우 처리

            // stopCompOp가 GREATEST인 경우 또는 LEAST인 경우,
            if (!(stopCompOp & SM_EOF) && !(stopCompOp & SM_BOF)) {
                cmp = edubtm_KeyCompare(kdesc, &cursor->key, stopKval);

                if (cmp == EQUAL && !(stopCompOp & SM_EQ)) {
                    cursor->flag = CURSOR_EOS;
                }
                else if (cmp == GREATER && stopCompOp & SM_LT) {
                    cursor->flag = CURSOR_EOS;
                }
                else if (cmp == LESS && stopCompOp & SM_GT) {
                    cursor->flag = CURSOR_EOS;
                }
            }
        }
    }

    else {
        ERRB1(eBADBTREEPAGE_BTM, root, PAGE_BUF);
    }

    if (!IS_NILPAGEID(prevPid)) {
        e = BfM_FreeTrain(&prevPid, PAGE_BUF);
        if (e < eNOERROR) ERRB1(e, root, PAGE_BUF);
    }
    
    if (!IS_NILPAGEID(nextPid)) {
        e = BfM_FreeTrain(&nextPid, PAGE_BUF);
        if (e < eNOERROR) ERRB1(e, root, PAGE_BUF);
    }

    e = BfM_FreeTrain(root, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    return(eNOERROR);
    
} /* edubtm_Fetch() */

