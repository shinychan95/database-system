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
 * Module: EduOM_PrevObject.c
 *
 * Description: 
 *  Return the previous object of the given current object.
 *
 * Exports:
 *  Four EduOM_PrevObject(ObjectID*, ObjectID*, ObjectID*, ObjectHdr*)
 */


#include "EduOM_common.h"
#include "BfM.h"
#include "EduOM_Internal.h"

/*@================================
 * EduOM_PrevObject()
 *================================*/
/*
 * Function: Four EduOM_PrevObject(ObjectID*, ObjectID*, ObjectID*, ObjectHdr*)
 *
 * Description: 
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  Return the previous object of the given current object. Find the object in
 *  the same page which has the current object and  if there  is no previous
 *  object in the same page, find it from the previous page.
 *  If the current object is NULL, return the last object of the file.
 *
 * Returns:
 *  error code
 *    eBADCATALOGOBJECT_OM
 *    eBADOBJECTID_OM
 *    some errors caused by function calls
 *
 * Side effect:
 *  1) parameter prevOID
 *     prevOID is filled with the previous object's identifier
 *  2) parameter objHdr
 *     objHdr is filled with the previous object's header
 */
Four EduOM_PrevObject(
    ObjectID *catObjForFile,	/* IN informations about a data file */
    ObjectID *curOID,		/* IN a ObjectID of the current object */
    ObjectID *prevOID,		/* OUT the previous object of a current object */
    ObjectHdr*objHdr)		/* OUT the object header of previous object */
{
    Four e;			/* error */
    Two  i;			/* index */
    Four offset;		/* starting offset of object within a page */
    PageID pid;			/* a page identifier */
    PageNo pageNo;		/* a temporary var for previous page's PageNo */
    SlottedPage *apage;		/* a pointer to the data page */
    Object *obj;		/* a pointer to the Object */
    PhysicalFileID pFid;	/* file in which the objects are located */
    SlottedPage *catPage;	/* buffer page containing the catalog object */
    sm_CatOverlayForData *catEntry; /* overlay structure for catalog object access */



    /*@ parameter checking */
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);
    
    if (prevOID == NULL) ERR(eBADOBJECTID_OM);

    MAKE_PHYSICALFILEID(pFid, catObjForFile->volNo, catObjForFile->pageNo);
    e = BfM_GetTrain(&pFid, &catPage, PAGE_BUF);
    if(e < eNOERROR) ERR(e);
    GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);


    // 파라미터로 주어진 curOID가 NULL인 경우
    if (curOID == NULL) {
        // File의 마지막 page의 slot array 상에서의 마지막 object의 ID를 반환함
        pageNo = catEntry->lastPage;
        MAKE_PAGEID(pid, catObjForFile->volNo, pageNo);
        e = BfM_GetTrain(&pid, &apage, PAGE_BUF);
        if (e < eNOERROR) ERRB1(e, &pFid, PAGE_BUF);

        i = apage->header.nSlots - 1;
        offset = apage->slot[-i].offset;
        obj = &(apage->data[offset]);
        
        MAKE_OBJECTID(*prevOID, apage->header.pid.volNo, apage->header.pid.pageNo, i, apage->slot[-i].unique);
        objHdr = &obj->header;
        
        BfM_FreeTrain(&pFid, PAGE_BUF);
        BfM_FreeTrain(&pid, PAGE_BUF);

        return(eNOERROR);
    }
    // 파라미터로 주어진 curOID가 NULL이 아닌 경우
    else {
        // curOID에 대응하는 object를 탐색함
        MAKE_PAGEID(pid, curOID->volNo, curOID->pageNo);
        e = BfM_GetTrain(&pid, &apage, PAGE_BUF);
        if (e < eNOERROR) ERRB1(e, &pFid, PAGE_BUF);   

        if (!IS_VALID_OBJECTID(curOID, apage)) ERRB2(eBADOBJECTID_OM, &pFid, &pid, PAGE_BUF);

        // 페이지 내 첫번째 object의 slot number를 구한다.
        for (i = 0; i < apage->header.nSlots; i++) {
            if (apage->slot[-i].offset != EMPTYSLOT) break;
        }

        // Slot array 상에서, 탐색한 object의 이전 object의 ID를 반환함
        // 탐색한 object가 page의 첫 번째 object인 경우
        if (i == curOID->slotNo) {
            // file의 첫번째 page인 경우
            if (catEntry->firstPage == apage->header.pid.pageNo) {
                return(EOS);
            }
            // file의 첫번째 page가 아닌 경우
            else {
                // 이전 page의 마지막 object의 ID를 반환함
                MAKE_PAGEID(pid, curOID->volNo, apage->header.prevPage);
                e = BfM_GetTrain(&pid, &apage, PAGE_BUF);
                if (e < eNOERROR) ERRB1(e, &pFid, PAGE_BUF);

                i = apage->header.nSlots - 1;
                offset = apage->slot[-i].offset;
                obj = &(apage->data[offset]);
                
                MAKE_OBJECTID(*prevOID, apage->header.pid.volNo, apage->header.pid.pageNo, i, apage->slot[-i].unique);
                objHdr = &obj->header;
                
                BfM_FreeTrain(&pFid, PAGE_BUF);
                BfM_FreeTrain(&pid, PAGE_BUF);

                return(eNOERROR);
            }
        }
        // 탐색한 object가 page의 첫번째 object가 아닌 경우,
        else {
            for (i = curOID->slotNo - 1; i >= 0; i--) {
                if (apage->slot[-i].offset != EMPTYSLOT) {
                    offset = apage->slot[-i].offset;
                    obj = &(apage->data[offset]);

                    MAKE_OBJECTID(*prevOID, apage->header.pid.volNo, apage->header.pid.pageNo, i, apage->slot[-i].unique);
                    objHdr = &obj->header;
                    
                    BfM_FreeTrain(&pFid, PAGE_BUF);
                    BfM_FreeTrain(&pid, PAGE_BUF);

                    return(eNOERROR);
                }
            }
        }
    }

    // 모든 경우에 해당하지 않는다면, 우선 eBADPARAMETER_OM를 반환한다.
    return(eBADPARAMETER_OM);
    
} /* EduOM_PrevObject() */
