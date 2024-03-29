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
 * Module: EduBtM_FetchNext.c
 *
 * Description:
 *  Find the next ObjectID satisfying the given condition. The current ObjectID
 *  is specified by the 'current'.
 *
 * Exports:
 *  Four EduBtM_FetchNext(PageID*, KeyDesc*, KeyValue*, Four, BtreeCursor*, BtreeCursor*)
 */


#include <string.h>
#include "EduBtM_common.h"
#include "BfM.h"
#include "EduBtM_Internal.h"


/*@ Internal Function Prototypes */
Four edubtm_FetchNext(KeyDesc*, KeyValue*, Four, BtreeCursor*, BtreeCursor*);



/*@================================
 * EduBtM_FetchNext()
 *================================*/
/*
 * Function: Four EduBtM_FetchNext(PageID*, KeyDesc*, KeyValue*,
 *                              Four, BtreeCursor*, BtreeCursor*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Fetch the next ObjectID satisfying the given condition.
 * By the B+ tree structure modification resulted from the splitting or merging
 * the current cursor may point to the invalid position. So we should adjust
 * the B+ tree cursor before using the cursor.
 *
 * Returns:
 *  error code
 *    eBADPARAMETER_BTM
 *    eBADCURSOR
 *    some errors caused by function calls
 * 
 * 한글 설명:
 *  B+ tree 색인에서 검색 조건을 만족하는 현재 object의 다음 object를 검색하고, 검색된 object를 가리키는 cursor를 반환함
 * 
 * 관련 함수:
 *  - edubtm_FetchNext(), 
 *  - edubtm_KeyCompare(), 
 *  - BfM_GetTrain(), 
 *  - BfM_FreeTrain()
 */
Four EduBtM_FetchNext(
    PageID                      *root,          /* IN root page's PageID */
    KeyDesc                     *kdesc,         /* IN key descriptor */
    KeyValue                    *kval,          /* IN key value of stop condition */
    Four                        compOp,         /* IN comparison operator of stop condition */
    BtreeCursor                 *current,       /* IN current B+ tree cursor */
    BtreeCursor                 *next)          /* OUT next B+ tree cursor */
{
    int							i;
    Four                        e;              /* error number */
    Four                        cmp;            /* comparison result */
    Two                         slotNo;         /* slot no. of a leaf page */
    Two                         oidArrayElemNo; /* element no. of the array of ObjectIDs */
    Two                         alignedKlen;    /* aligned length of key length */
    PageID                      overflow;       /* temporary PageID of an overflow page */
    Boolean                     found;          /* search result */
    ObjectID                    *oidArray;      /* array of ObjectIDs */
    BtreeLeaf                   *apage;         /* pointer to a buffer holding a leaf page */
    BtreeOverflow               *opage;         /* pointer to a buffer holding an overflow page */
    btm_LeafEntry               *entry;         /* pointer to a leaf entry */
    BtreeCursor                 tCursor;        /* a temporary Btree cursor */
  
    
    /*@ check parameter */
    if (root == NULL || kdesc == NULL || kval == NULL || current == NULL || next == NULL)
	ERR(eBADPARAMETER_BTM);
    
    /* Is the current cursor valid? */
    if (current->flag != CURSOR_ON && current->flag != CURSOR_EOS)
		ERR(eBADCURSOR);
    
    if (current->flag == CURSOR_EOS) return(eNOERROR);
    
    /* Error check whether using not supported functionality by EduBtM */
    for (i = 0; i < kdesc->nparts; i++) {
        if (kdesc->kpart[i].type != SM_INT && kdesc->kpart[i].type != SM_VARSTRING) {
            ERR(eNOTSUPPORTED_EDUBTM);
        }
    }

    e = edubtm_FetchNext(kdesc, kval, compOp, current, next);
    if (e < eNOERROR) ERR(e);

    
    return(eNOERROR);
    
} /* EduBtM_FetchNext() */



/*@================================
 * edubtm_FetchNext()
 *================================*/
/*
 * Function: Four edubtm_FetchNext(KeyDesc*, KeyValue*, Four,
 *                              BtreeCursor*, BtreeCursor*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Get the next item. We assume that the current cursor is valid; that is.
 *  'current' rightly points to an existing ObjectID.
 *
 * Returns:
 *  Error code
 *    eBADCOMPOP_BTM
 *    some errors caused by function calls
 * 
 * 한글 설명:
 *  B+ tree 색인에서 검색 조건을 만족하는 현재 leaf index entry의 다음 leaf index entry를 검색하고, 
 *  검색된 leaf index entry를 가리키는 cursor를 반환함. 검색 조건이 SM_GT, SM_GE일 경우 
 *  key 값이 작아지는 방향으로 backward scan을 하며, 그 외의 경우 key 값이 커지는 방향으로 forward scan을 한다
 * 
 * 관련 함수:
 *  - edubtm_KeyCompare(), 
 *  - BfM_GetTrain(), 
 *  - BfM_FreeTrain()
 */
Four edubtm_FetchNext(
    KeyDesc  		*kdesc,		/* IN key descriptor */
    KeyValue 		*kval,		/* IN key value of stop condition */
    Four     		compOp,		/* IN comparison operator of stop condition */
    BtreeCursor 	*current,	/* IN current cursor */
    BtreeCursor 	*next)		/* OUT next cursor */
{
    Four 		e;		/* error number */
    Four 		cmp;		/* comparison result */
    Two 		alignedKlen;	/* aligned length of a key length */
    PageID 		leaf;		/* temporary PageID of a leaf page */
    PageID 		overflow;	/* temporary PageID of an overflow page */
    ObjectID 		*oidArray;	/* array of ObjectIDs */
    BtreeLeaf 		*apage;		/* pointer to a buffer holding a leaf page */
    BtreeOverflow 	*opage;		/* pointer to a buffer holding an overflow page */
    btm_LeafEntry 	*entry;		/* pointer to a leaf entry */    
    
    
    /* Error check whether using not supported functionality by EduBtM */
    int i;
    for (i = 0; i < kdesc->nparts; i++) {
        if (kdesc->kpart[i].type != SM_INT && kdesc->kpart[i].type != SM_VARSTRING) {
            ERR(eNOTSUPPORTED_EDUBTM);
        }
    }

    // 검색 조건을 만족하는 다음 leaf index entry를 검색함
    leaf = current->leaf;
    e = BfM_GetTrain((TrainID*)&leaf, (char**)&apage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    *next = *current;

    // 증가하는 경우,
    if (compOp & SM_LT || compOp & SM_EOF) {
        if (next->slotNo == apage->hdr.nSlots - 1) {
            if (apage->hdr.nextPage == NIL) {
                next->flag = CURSOR_EOS;
            }
            else {
                next->leaf.pageNo = apage->hdr.nextPage;

                e = BfM_FreeTrain((TrainID*)&leaf, PAGE_BUF);
                if (e < eNOERROR) ERR(e);

                leaf.pageNo = next->leaf.pageNo;
                e = BfM_GetTrain((TrainID*)&leaf, (char**)&apage, PAGE_BUF);

                next->slotNo = 0;

                entry = (btm_LeafEntry*)&apage->data[next->slotNo];
            
                next->oid = *(ObjectID*)&entry->kval[ALIGNED_LENGTH(entry->klen)];
                next->key.len = entry->klen;
                memcpy(&(next->key.val[0]), &entry->kval, entry->klen);
            }
        }
        else {
            next->slotNo += 1;

            entry = (btm_LeafEntry*)&apage->data[next->slotNo];
            
            next->oid = *(ObjectID*)&entry->kval[ALIGNED_LENGTH(entry->klen)];
            next->key.len = entry->klen;
	        memcpy(&(next->key.val[0]), &entry->kval, entry->klen);
        }
    }
    // 감소하는 경우,
    else if (compOp & SM_GT || compOp & SM_BOF) {
        if (next->slotNo == 0) {
            if (apage->hdr.prevPage == NIL) {
                next->flag = CURSOR_EOS;
            }
            else {
                next->leaf.pageNo = apage->hdr.prevPage;

                e = BfM_FreeTrain((TrainID*)&leaf, PAGE_BUF);
                if (e < eNOERROR) ERR(e);

                leaf.pageNo = next->leaf.pageNo;
                e = BfM_GetTrain((TrainID*)&leaf, (char**)&apage, PAGE_BUF);

                next->slotNo = apage->hdr.nSlots-1;

                entry = (btm_LeafEntry*)&apage->data[next->slotNo];
            
                next->oid = *(ObjectID*)&entry->kval[ALIGNED_LENGTH(entry->klen)];
                next->key.len = entry->klen;
                memcpy(&(next->key.val[0]), &entry->kval, entry->klen);
            }
        }
        else {
            next->slotNo -= 1;

            entry = (btm_LeafEntry*)&apage->data[next->slotNo];
            
            next->oid = *(ObjectID*)&entry->kval[ALIGNED_LENGTH(entry->klen)];
            next->key.len = entry->klen;
	        memcpy(&(next->key.val[0]), &entry->kval, entry->klen);
        }
    }
    else {
        next->flag = CURSOR_EOS;
    }

    if (!(compOp == SM_EOF) && !(compOp == SM_BOF)) {
        cmp = edubtm_KeyCompare(kdesc, kval, &next->key);
        if (1 << cmp & compOp)
            next->flag = CURSOR_ON;
        else
            next->flag = CURSOR_EOS;
    }

    e = BfM_FreeTrain((TrainID*)&leaf, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    
    return(eNOERROR);
    
} /* edubtm_FetchNext() */
