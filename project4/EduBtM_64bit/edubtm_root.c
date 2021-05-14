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
 * Module: edubtm_root.c
 *
 * Description : 
 *  This file has two routines which are concerned with the changing the
 *  current root node. When an overflow or a underflow occurs in the root page
 *  the root node should be changed. But we don't change the root node to
 *  the new page. The old root page is used as the new root node; thus the
 *  root page is fixed always.
 *
 * Exports:
 *  Four edubtm_root_insert(ObjectID*, PageID*, InternalItem*)
 */


#include <string.h>
#include "EduBtM_common.h"
#include "Util.h"
#include "BfM.h"
#include "EduBtM_Internal.h"



/*@================================
 * edubtm_root_insert()
 *================================*/
/*
 * Function: Four edubtm_root_insert(ObjectID*, PageID*, InternalItem*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  This routine is called when a new entry was inserted into the root page and
 *  it was splitted two pages, 'root' and 'item->pid'. The new root should be
 *  made by the given root Page IDentifier and the sibling entry 'item'.
 *  We make it a rule to fix the root page; so a new page is allocated and
 *  the root node is copied into the newly allocated page. The root node
 *  is changed so that it points to the newly allocated node and the 'item->pid'.
 *
 * Returns:
 *  Error code
 *    some errors caused by function calls
 * 
 * 한글 설명:
 *  Root page가 split 된 B+ tree 색인을 위한 새로운 root page를 생성함
 * 
 * 관련 함수:
 *  - edubtm_InitInternal(), 
 *  - btm_AllocPage(), 
 *  - BfM_GetTrain(), 
 *  - BfM_GetNewTrain(), 
 *  - BfM_FreeTrain(), 
 *  - BfM_SetDirty()
 */
Four edubtm_root_insert(
    ObjectID     *catObjForFile, /* IN catalog object of B+ tree file */
    PageID       *root,		 /* IN root Page IDentifier */
    InternalItem *item)		 /* IN Internal item which will be the unique entry of the new root */
{
    Four      e;		/* error number */
    PageID    newPid;		/* newly allocated page */
    PageID    nextPid;		/* PageID of the next page of root if root is leaf */
    BtreePage *rootPage;	/* pointer to a buffer holding the root page */
    BtreePage *newPage;		/* pointer to a buffer holding the new page */
    BtreeLeaf *nextPage;	/* pointer to a buffer holding next page of root */
    btm_InternalEntry *entry;	/* an internal entry */
    Two       entryLen;         /* length of an entry */
    Boolean   isTmp;

    // 새로운 page를 할당 받음
    e = btm_AllocPage(catObjForFile, root, &newPid);
    if (e < eNOERROR) ERR(e);
    
    e = BfM_GetTrain(root, (char**)&rootPage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    
    e = BfM_GetNewTrain(&newPid, (char**)&newPage, PAGE_BUF);
    if (e < eNOERROR) ERRB1(e, root, PAGE_BUF);

    // 기존 root page를 할당 받은 page로 복사함
    memcpy(newPage, rootPage, PAGESIZE);
    newPage->any.hdr.type &= ~ROOT;
    newPage->any.hdr.pid = newPid;

    // 기존 root page를 새로운 root page로서 초기화함
    rootPage->bi.hdr.flags= BTREE_PAGE_TYPE;
    rootPage->bi.hdr.free = 0;
    rootPage->bi.hdr.reserved = 0;
    rootPage->bi.hdr.type = INTERNAL | ROOT;
    rootPage->bi.hdr.unused = 0;

    // 할당 받은 page와 root page split으로 생성된 page가 새로운 root page의 자식 page들이 되도록 설정함
    MAKE_PAGEID(nextPid, newPid.volNo, item->spid);
    e = BfM_GetTrain(&nextPid, (char**)&nextPage, PAGE_BUF);
    if (e < eNOERROR) ERRB1(e, root, PAGE_BUF);

    // Split으로 생성된 page를 가리키는 internal index entry를 새로운 root page에 삽입함
    entryLen = sizeof(ShortPageID) + sizeof(Two) + ALIGNED_LENGTH(item->klen);
    entry = (btm_InternalEntry*)&rootPage->bi.data[0];
    memcpy(entry, item, entryLen);
    rootPage->bi.slot[0] = rootPage->bi.hdr.free;
    rootPage->bi.hdr.free += entryLen;
    rootPage->bi.hdr.nSlots = 1;

    // 새로운 root page의 header의 p0 변수에 할당 받은 page의 번호를 저장함
    rootPage->bi.hdr.p0 = newPid.pageNo;
    
    // 새로운 root page의 두 자식 page들이 leaf인 경우, 두 자식 page들간의 doubly linked list를 설정함
    if (newPage->any.hdr.type & LEAF) {
        nextPage->hdr.prevPage = newPid.pageNo;
        newPage->bl.hdr.nextPage = nextPid.pageNo;
    }

    e = BfM_SetDirty(&nextPid, PAGE_BUF);
    if (e < eNOERROR) ERRB3(e, root, PAGE_BUF, &newPid, PAGE_BUF, &nextPid, PAGE_BUF);
    
    e = BfM_SetDirty(&newPid, PAGE_BUF);
    if (e < eNOERROR) ERRB3(e, root, PAGE_BUF, &newPid, PAGE_BUF, &nextPid, PAGE_BUF);

    e = BfM_SetDirty(root, PAGE_BUF);
    if (e < eNOERROR) ERRB3(e, root, PAGE_BUF, &newPid, PAGE_BUF, &nextPid, PAGE_BUF);

    e = BfM_FreeTrain(&nextPid, PAGE_BUF);
    if (e < eNOERROR) ERRB2(e, root, PAGE_BUF, &newPid, PAGE_BUF);

    e = BfM_FreeTrain(&newPid, PAGE_BUF);
    if (e < eNOERROR) ERRB1(e, root, PAGE_BUF);
    
    e = BfM_FreeTrain(root, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    
    return(eNOERROR);
    
} /* edubtm_root_insert() */
