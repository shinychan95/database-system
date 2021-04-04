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
 * Module : EduOM_DestroyObject.c
 * 
 * Description : 
 *  EduOM_DestroyObject() destroys the specified object.
 *
 * Exports:
 *  Four EduOM_DestroyObject(ObjectID*, ObjectID*, Pool*, DeallocListElem*)
 */

#include "EduOM_common.h"
#include "Util.h"		/* to get Pool */
#include "RDsM.h"
#include "BfM.h"		/* for the buffer manager call */
#include "LOT.h"		/* for the large object manager call */
#include "EduOM_Internal.h"

/*@================================
 * EduOM_DestroyObject()
 *================================*/
/*
 * Function: Four EduOM_DestroyObject(ObjectID*, ObjectID*, Pool*, DeallocListElem*)
 * 
 * Description : 
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  (1) What to do?
 *  EduOM_DestroyObject() destroys the specified object. The specified object
 *  will be removed from the slotted page. The freed space is not merged
 *  to make the contiguous space; it is done when it is needed.
 *  The page's membership to 'availSpaceList' may be changed.
 *  If the destroyed object is the only object in the page, then deallocate
 *  the page.
 *
 *  (2) How to do?
 *  a. Read in the slotted page
 *  b. Remove this page from the 'availSpaceList'
 *  c. Delete the object from the page
 *  d. Update the control information: 'unused', 'freeStart', 'slot offset'
 *  e. IF no more object in this page THEN
 *	   Remove this page from the filemap List
 *	   Dealloate this page
 *    ELSE
 *	   Put this page into the proper 'availSpaceList'
 *    ENDIF
 * f. Return
 *
 * Returns:
 *  error code
 *    eBADCATALOGOBJECT_OM
 *    eBADOBJECTID_OM
 *    eBADFILEID_OM
 *    some errors caused by function calls
 * 
 * 설명:
 *  File을 구성하는 page에서 object를 삭제함
 * 
 * 관련 함수:
 *  1. om_FileMapDeletePage() - Page를 file 구성 page들로 이루어진 list에서 삭제함
 *  2. om_PutInAvailSpaceList()
 *  3. om_RemoveFromAvailSpaceList()
 *  4. BfM_GetTrain()
 *  5. BfM_FreeTrain()
 *  6. BfM_SetDirty()
 *  7. Util_getElementFromPool() - Pool에서 새로운 dealloc list element 한 개를 위한 메모리 공간을 할당 받고, 
                                   할당 받은 메모리 공간에 대한 포인터를 반환함
 */
Four EduOM_DestroyObject(
    ObjectID *catObjForFile,	/* IN file containing the object */
    ObjectID *oid,		/* IN object to destroy */
    Pool     *dlPool,		/* INOUT pool of dealloc list elements */
    DeallocListElem *dlHead)	/* INOUT head of dealloc list */
{
    Four        e;		/* error number */
    Two         i;		/* temporary variable */
    FileID      fid;		/* ID of file where the object was placed */
    PageID	    pid;		/* page on which the object resides */
    SlottedPage *apage;		/* pointer to the buffer holding the page */
    Four        offset;		/* start offset of object in data area */
    Object      *obj;		/* points to the object in data area */
    Four        alignedLen;	/* aligned length of object */
    Boolean     last;		/* indicates the object is the last one */
    SlottedPage *catPage;	/* buffer page containing the catalog object */
    sm_CatOverlayForData *catEntry; /* overlay structure for catalog object access */
    DeallocListElem *dlElem;	    /* pointer to element of dealloc list */
    PhysicalFileID pFid;	        /* physical ID of file */
    

    /*@ Check parameters. */
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);

    if (oid == NULL) ERR(eBADOBJECTID_OM);

    // sm_CatOverlayForData에 해당하는 page를 buffer에 fix 한다.
    MAKE_PHYSICALFILEID(pFid, catObjForFile->volNo, catObjForFile->pageNo);
    e = BfM_GetTrain(&pFid, &catPage, PAGE_BUF);
    if(e < eNOERROR) ERR(e);
    GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);
    fid = catEntry->fid;

    // 해당 object가 존재하는 page를 buffer에 fix 한다.
    MAKE_PAGEID(pid, oid->volNo, oid->pageNo);
    e = BfM_GetTrain(&pid, &apage, PAGE_BUF);
    if (e < eNOERROR) ERRB1(e, &pFid, PAGE_BUF); // 만약 해당하는 page가 존재하지 않는다면 에러 발생

    // 해당 ObjectID가 valid 한지 아닌지 체크한다.
    if (!IS_VALID_OBJECTID(oid, apage)) ERRB2(eBADOBJECTID_OM, &pFid, &pid, PAGE_BUF);
 
    // 삭제할 object가 저장된 page를 현재 available space list에서 삭제함
    e = om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);
    if (e < eNOERROR) ERRB2(e, &pFid, &pid, PAGE_BUF);

    // object 관련 변수들의 값 저장
    offset = apage->slot[-(oid)->slotNo].offset;
    obj = &apage->data[offset];
    alignedLen = ALIGNED_LENGTH(obj->header.length);

    // 삭제할 object에 대응하는 slot을 사용하지 않는 빈 slot으로 설정함
    apage->slot[-(oid)->slotNo].offset = EMPTYSLOT;
    apage->slot[-(oid)->slotNo].unique = 0;

    
    // Page header를 갱신함
    if (oid->slotNo == apage->header.nSlots - 1) {
        apage->header.nSlots--;
    }

    if (offset + sizeof(ObjectHdr) + alignedLen == apage->header.free) {
        apage->header.free -= sizeof(ObjectHdr) + alignedLen;
    }
    else {
        apage->header.unused += sizeof(ObjectHdr) + alignedLen;
    }


    // 삭제된 object가 page의 유일한 object이고, 해당 page가 file의 첫 번째 page가 아닌 경우,
    if (apage->header.nSlots == 0 && apage->header.prevPage != NIL) {
        // Page를 file 구성 page들로 이루어진 list에서 삭제함
        e = om_FileMapDeletePage(catObjForFile, &pid);
        if (e < eNOERROR) ERRB2(e, &pFid, &pid, PAGE_BUF);

        // 해당 page를 deallocate 함
        e = Util_getElementFromPool(dlPool, &dlElem);
        if (e < eNOERROR) ERRB2(e, &pFid, &pid, PAGE_BUF);

        dlElem->elem.pFid = pFid;
        dlElem->elem.pid = pid;
        dlElem->next = dlHead;
        dlHead = dlElem;
        dlElem->type = DL_PAGE;
    }
    // 삭제된 object가 page의 유일한 object가 아니거나, 해당 page가 file의 첫 번째 page인 경우, 
    else {
        // Page를 알맞은 available space list에 삽입함
        e = om_PutInAvailSpaceList(catObjForFile, &pid, apage);
        if (e < eNOERROR) ERRB2(e, &pFid, &pid, PAGE_BUF);
    }

    // 모든 transaction들은 page/train access를 마치고 해당 page/train을 buffer에서 unfix 해야 함
    BfM_FreeTrain(&pFid, PAGE_BUF);
    BfM_FreeTrain(&pid, PAGE_BUF);
    
    return(eNOERROR);
    
} /* EduOM_DestroyObject() */
