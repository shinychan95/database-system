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
 * Module: edubtm_InitPage.c
 *
 * Description :
 *  This file has three functions which initialize Btree pages.  A Btree page
 *  should be initialized by one of these functions before it is used.
 *
 * Exports:
 *  Four edubtm_InitInternal(PageID*, Boolean)
 *  Four edubtm_InitLeaf(PageID*, Boolean)
 */


#include "EduBtM_common.h"
#include "BfM.h"
#include "EduBtM_Internal.h"



/*@================================
 * edubtm_InitInternal()
 *================================*/
/*
 * Function: Four edubtm_InitInternal(PageID*, Boolean)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Initialize as an internal page.  If 'root' is TRUE, this page may be
 *  initialized as a root.
 *
 * Returns:
 *  Error code
 *    some errors caused by function calls
 * 
 * 한글 설명:
 *  Page를 B+ tree 색인의 internal page로 초기화함
 */
Four edubtm_InitInternal(
    PageID  *internal,		/* IN the PageID to be initialized */
    Boolean root,		/* IN Is it root ? */
    Boolean isTmp)              /* IN Is it temporary ? - COOKIE12FEB98 (Not Use) */
{
    Four e;			/* error number */
    BtreeInternal *page;	/* a page pointer */

    e = BfM_GetNewTrain(internal, (char**)&page, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    // Page header를 internal page로 초기화함
    MAKE_PAGEID(page->hdr.pid, internal->pageNo, internal->volNo);
    page->hdr.flags = BTREE_PAGE_TYPE;
    if (root) page->hdr.type = ROOT;
    else page->hdr.type = INTERNAL;
    page->hdr.reserved = 0;
    page->hdr.p0 = NIL;
    page->hdr.nSlots = 0;
    page->hdr.free = 0;
    page->hdr.unused = 0;

    e = BfM_SetDirty(internal, PAGE_BUF);
    if (e < eNOERROR) ERRB1(e, internal, PAGE_BUF);

    e = BfM_FreeTrain(internal, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    return(eNOERROR);
    
}  /* edubtm_InitInternal() */



/*@================================
 * edubtm_InitLeaf()
 *================================*/
/*
 * Function: Four edubtm_InitLeaf(PageID*, Boolean)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Initialize as a leaf page. If 'root' is TRUE, this page may be initialize
 *  as a root.
 *
 * Returns:
 *  Error code
 *    some errors caused by function calls
 * 
 * 한글 설명:
 *  Page를 B+ tree 색인의 leaf page로 초기화함
 * 
 * 관련 함수:
 *  - BfM_GetNewTrain(), 
 *  - BfM_FreeTrain(), 
 *  - BfM_SetDirty()
 */
Four edubtm_InitLeaf(
    PageID *leaf,		/* IN the PageID to be initialized */
    Boolean root,		/* IN Is it root ? */
    Boolean isTmp)      /* IN Is it temporary ? (Not Use) */

{
    Four e;			/* error number */
    BtreeLeaf *page;		/* a page pointer */

    e = BfM_GetNewTrain(leaf, (char**)&page, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    // Page header를 leaf page로 초기화함
    MAKE_PAGEID(page->hdr.pid, leaf->pageNo, leaf->volNo);
    page->hdr.flags = BTREE_PAGE_TYPE;
    if (root) page->hdr.type = ROOT;
    else page->hdr.type = LEAF;
    page->hdr.reserved = 0;
    page->hdr.nSlots = 0;
    page->hdr.free = 0;
    page->hdr.prevPage = NIL;
    page->hdr.nextPage = NIL;
    page->hdr.unused = 0;
    

    e = BfM_SetDirty(leaf, PAGE_BUF);
    if (e < eNOERROR) ERRB1(e, leaf, PAGE_BUF);
    
    e = BfM_FreeTrain(leaf, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    
    return(eNOERROR);
    
}  /* edubtm_InitLeaf() */
