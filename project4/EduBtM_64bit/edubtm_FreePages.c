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
 * Module: edubtm_FreePages.c
 *
 * Description :
 *  Free all pages which were related with the given page. 
 *
 * Exports:
 *  Four edubtm_FreePages(FileID*, PageID*, Pool*, DeallocListElem*)
 */


#include "EduBtM_common.h"
#include "Util.h"
#include "BfM.h"
#include "EduBtM_Internal.h"



/*@================================
 * edubtm_FreePages()
 *================================*/
/*
 * Function: Four edubtm_FreePages(FileID*, PageID*, Pool*, DeallocListElem*)
 *
 * Description :
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Free all pages which were related with the given page. If the given page
 *  is an internal page, recursively free all child pages before it is freed.
 *  In a leaf page, examine all leaf items whether it has an overflow page list
 *  before it is freed. If it has, recursively call itself by using the first
 *  overflow page. In an overflow page, it recursively calls itself if the
 *  'nextPage' exist.
 * 
 *  + 이번 과제에서는 overflow page는 고려하지 않는다.
 *
 * Returns:
 *  error code
 *    eBADBTREEPAGE_BTM
 *    some errors caused by function calls
 * 
 * 한글 설명:
 *  B+ tree 색인 page를 deallocate 함 (자식 page들도 모두 deallocate)
 * 
 * 관련 함수:
 *  - BfM_GetNewTrain(), 
 *  - BfM_FreeTrain(), 
 *  - BfM_SetDirty(), 
 *  - Util_getElementFromPool()
 */
Four edubtm_FreePages(
    PhysicalFileID      *pFid,          /* IN FileID of the Btree file */
    PageID              *curPid,        /* IN The PageID to be freed */
    Pool                *dlPool,        /* INOUT pool of dealloc list elements */
    DeallocListElem     *dlHead)        /* INOUT head of the dealloc list */
{
    Four                e;              /* error number */
    Two                 i;              /* index */
    Two                 alignedKlen;    /* aligned length of the key length */
    PageID              tPid;           /* a temporary PageID */
    BtreePage           *apage;         /* a page pointer */
    Two                 iEntryOffset;   /* starting offset of an internal entry */
    Two                 lEntryOffset;   /* starting offset of a leaf entry */
    btm_InternalEntry   *iEntry;        /* an internal entry */
    btm_LeafEntry       *lEntry;        /* a leaf entry */
    DeallocListElem     *dlElem;        /* an element of dealloc list */

    if (curPid == NULL) ERR(eBADBTREEPAGE_BTM);

    e = BfM_GetTrain(curPid, (char**)&apage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    // 주어진 page의 모든 자식 page들을 재귀적으로 deallocate 함
    if (apage->any.hdr.type & INTERNAL) {
        MAKE_PAGEID(tPid, curPid->volNo, apage->bi.hdr.p0);
        e = edubtm_FreePages(pFid, &tPid, dlPool, dlHead);
        if (e < eNOERROR) ERRB1(e, curPid, PAGE_BUF);

        for (i = 0; i < apage->bi.hdr.nSlots; i++) {
            iEntry = (btm_InternalEntry*)&(apage->bi.data[apage->bi.slot[-i]]);
            MAKE_PAGEID(tPid, curPid->volNo, iEntry->spid);
            e = edubtm_FreePages(pFid, &tPid, dlPool, dlHead);
            if (e < eNOERROR) ERRB1(e, curPid, PAGE_BUF);
        }
    }
    else if (apage->any.hdr.type & LEAF == FALSE) {
        ERRB1(eBADPAGETYPE_BTM, curPid, PAGE_BUF);
    }
    
    // 파라미터로 주어진 page를 deallocate 함
    // Page header의 type에서 해당 page가 deallocate 될 page임을 나타내는 bit를 set 및 나머지 bit들을 unset 함
    apage->any.hdr.type = FREEPAGE;
    if (apage->any.hdr.type & INTERNAL) apage->bi.hdr.type = FREEPAGE;
    else apage->bl.hdr.type = FREEPAGE;
    
    // 파라미터로 주어진 dlPool에서 새로운 dealloc list element 한 개를 할당 받음
    e = Util_getElementFromPool(dlPool, &dlElem);
    if (e < eNOERROR) ERRB1(e, curPid, PAGE_BUF);
    
    dlElem->type = DL_PAGE;
    dlElem->elem.pid = *curPid;
    dlElem->next = dlHead->next;
    dlHead->next = dlElem;

    e = BfM_SetDirty(curPid, PAGE_BUF);
    if (e < eNOERROR) ERRB1(e, curPid, PAGE_BUF);
    
    e = BfM_FreeTrain(curPid, PAGE_BUF);
    if (e < eNOERROR) ERR(e);


    
    return(eNOERROR);
    
}   /* edubtm_FreePages() */
