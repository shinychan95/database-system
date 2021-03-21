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
 * Module : EduOM_CreateObject.c
 * 
 * Description :
 *  EduOM_CreateObject() creates a new object near the specified object.
 *
 * Exports:
 *  Four EduOM_CreateObject(ObjectID*, ObjectID*, ObjectHdr*, Four, char*, ObjectID*)
 */

#include <string.h>
#include "EduOM_common.h"
#include "RDsM.h"		/* for the raw disk manager call */
#include "BfM.h"		/* for the buffer manager call */
#include "EduOM_Internal.h"

/*@================================
 * EduOM_CreateObject()
 *================================*/
/*
 * Function: Four EduOM_CreateObject(ObjectID*, ObjectID*, ObjectHdr*, Four, char*, ObjectID*)
 * 
 * Description :
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 * (1) What to do?
 * EduOM_CreateObject() creates a new object near the specified object.
 * If there is no room in the page holding the specified object,
 * it trys to insert into the page in the available space list. If fail, then
 * the new object will be put into the newly allocated page.
 *
 * (2) How to do?
 *	a. Read in the near slotted page
 *	b. See the object header
 *	c. IF large object THEN
 *	       call the large object manager's lom_ReadObject()
 *	   ELSE 
 *		   IF moved object THEN 
 *				call this function recursively
 *		   ELSE 
 *				copy the data into the buffer
 *		   ENDIF
 *	   ENDIF
 *	d. Free the buffer page
 *	e. Return
 *
 * Returns:
 *  error code
 *    eBADCATALOGOBJECT_OM
 *    eBADLENGTH_OM
 *    eBADUSERBUF_OM
 *    some error codes from the lower level
 *
 * Side Effects :
 *  0) A new object is created.
 *  1) parameter oid
 *     'oid' is set to the ObjectID of the newly created object.
 * 
 * 설명 : 
 *  File을 구성하는 page들 중 파라미터로 지정한 object와 같은 (또는 인접한) page에 
 *  새로운 object를 삽입하고, 삽입된 object의 ID를 반환함.
 * 
 * 관련 함수 :
 *  1. eduom_CreateObject()
 */
Four EduOM_CreateObject(
    ObjectID  *catObjForFile,	/* IN file in which object is to be placed */
    ObjectID  *nearObj,		/* IN create the new object near this object */
    ObjectHdr *objHdr,		/* IN from which tag is to be set */
    Four      length,		/* IN amount of data */
    char      *data,		/* IN the initial data for the object */
    ObjectID  *oid)		/* OUT the object's ObjectID */
{
    Four        e;		/* error number */
    ObjectHdr   objectHdr;	/* ObjectHdr with tag set from parameter */


    /*@ parameter checking */
    
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);
    
    if (length < 0) ERR(eBADLENGTH_OM);

    if (length > 0 && data == NULL) return(eBADUSERBUF_OM);

	/* Error check whether using not supported functionality by EduOM */
	if(ALIGNED_LENGTH(length) > LRGOBJ_THRESHOLD) ERR(eNOTSUPPORTED_EDUOM);
    
    // 삽입할 object의 header를 초기화함
    objectHdr.length = 0;
    objectHdr.properties = 0x0;
    if (objHdr == NULL) objectHdr.tag = objHdr->tag;
    else objectHdr.tag = 0;

    // eduom_CreateObject()를 호출하여 page에 object를 삽입하고, 삽입된 object의 ID를 반환함
    e = eduom_CreateObject(catObjForFile, nearObj, &objectHdr, length, data, oid);
    if(e < 0) ERR(e);

    return(eNOERROR);
}

/*@================================
 * eduom_CreateObject()
 *================================*/
/*
 * Function: Four eduom_CreateObject(ObjectID*, ObjectID*, ObjectHdr*, Four, char*, ObjectID*)
 *
 * Description :
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  eduom_CreateObject() creates a new object near the specified object; the near
 *  page is the page holding the near object.
 *  If there is no room in the near page and the near object 'nearObj' is not
 *  NULL, a new page is allocated for object creation (In this case, the newly
 *  allocated page is inserted after the near page in the list of pages
 *  consiting in the file).
 *  If there is no room in the near page and the near object 'nearObj' is NULL,
 *  it trys to create a new object in the page in the available space list. If
 *  fail, then the new object will be put into the newly allocated page(In this
 *  case, the newly allocated page is appended at the tail of the list of pages
 *  cosisting in the file).
 *
 * Returns:
 *  error Code
 *    eBADCATALOGOBJECT_OM
 *    eBADOBJECTID_OM
 *    some errors caused by fuction calls
 * 
 * 설명:
 *  File을 구성하는 page들 중 파라미터로 지정한 object와 같은 (또는 인접한) page에 
 *  새로운 object를 삽입하고, 삽입된 object의 ID를 반환함
 * 
 * 관련 함수:
 *  1. EduOM_CompactPage()
 *  
 *  2. om_GetUnique() - Page에서 사용할 unique 번호를 할당 받고, 해당 page의 header의 관련 정보를 갱신하고, 할당 받은 unique 번호를 반환함
 *  3. om_FileMapAddPage() - Page를 file 구성 page들로 이루어진 list에 삽입함
 *  4. om_PutInAvailSpaceList() - Page를 available space list에 삽입함
 *  5. om_RemoveFromAvailSpaceList() - Page를 available space list에서 삭제함
 * 
 *  6. RDsM_PageIdToExtNo() - Page가 속한 extent의 번호를 반환함
 *  7. RDsM_AllocTrains() - Disk에서 새로운 page (sizeOfTrain=1) 또는 train (sizeOfTrain>1)을 할당하고, 
 *                          할당된 page의 ID 또는 train의 첫 번째 page의 ID를 반환함
 *                          (Train: page 데이터 영역의 크기보다 큰 large object를 저장하기 위한 구조로서, EduOM에서는 사용하지 않음)
 *  
 *  8. BfM_GetTrain() - Page (sizeOfTrain=1) 또는 train (sizeOfTrain>1) 을 buffer에 fix 하고, 
 *                      해당 page/train에 대한 포인터를 반환함
 *                      (모든 transaction들은 page/train을 access하기 전에 해당 page/train을 buffer에 fix 해야 함)
 *  9. BfM_GetNewTrain() - Disk 상에서 할당되지 않은 새로운 page (sizeOfTrain=1) 또는 train (sizeOfTrain>1) 을 buffer에 fix 하고,
 *                         해당 page/train에 대한 포인터를 반환함
 *  10. BfM_FreeTrain() - Page (sizeOfTrain=1) 또는 train (sizeOfTrain>1) 을 buffer에서 unfix 함
 *                        (모든 transaction들은 page/train access를 마치고 해당 page/train을 buffer에서 unfix 해야 함)
 *  11. BfM_SetDirty() - Buffer에 저장된 page (sizeOfTrain=1) 또는 train (sizeOfTrain>1) 이 수정되었음을 표시하기 위해 DIRTY bit를 set 함
 * 
 */
Four eduom_CreateObject(
                        ObjectID	*catObjForFile,	/* IN file in which object is to be placed */
                        ObjectID 	*nearObj,	/* IN create the new object near this object */
                        ObjectHdr	*objHdr,	/* IN from which tag & properties are set */
                        Four	length,		/* IN amount of data */
                        char	*data,		/* IN the initial data for the object */
                        ObjectID	*oid)		/* OUT the object's ObjectID */
{
    Four        e;		/* error number */
    Four	neededSpace;	/* space needed to put new object [+ header] */
    SlottedPage *apage;		/* pointer to the slotted page buffer */
    Four        alignedLen;	/* aligned length of initial data */
    Boolean     needToAllocPage;/* Is there a need to alloc a new page? */
    PageID      pid;            /* PageID in which new object to be inserted */
    PageID      nearPid;
    Four        firstExt;	/* first Extent No of the file */
    Object      *obj;		/* point to the newly created object */
    Two         i;		/* index variable */
    sm_CatOverlayForData *catEntry; /* pointer to data file catalog information */
    SlottedPage *catPage;	/* pointer to buffer containing the catalog */
    FileID      fid;		/* ID of file where the new object is placed */
    Two         eff;		/* extent fill factor of file */
    Boolean     isTmp;
    PhysicalFileID pFid;
    
    
    /*@ parameter checking */
    
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);
    
    if (objHdr == NULL) ERR(eBADOBJECTID_OM);
    
    /* Error check whether using not supported functionality by EduOM */
    if(ALIGNED_LENGTH(length) > LRGOBJ_THRESHOLD) ERR(eNOTSUPPORTED_EDUOM);
    
    // Object 삽입을 위해 필요한 자유 공간의 크기를 계산함
    alignedLen = ALIGNED_LENGTH(length);
    neededSpace = sizeof(ObjectHdr) + alignedLen + sizeof(SlottedPageSlot);

    // Object를 삽입할 page를 선정함
    // 모든 transaction들은 page/train을 access하기 전에 해당 page/train을 buffer에 fix 해야 한다.
    // 1. sm_CatOverlayForData를 담기 위한 catPage
    MAKE_PHYSICALFILEID(pFid, catObjForFile->volNo, catObjForFile->pageNo);
    e = BfM_GetTrain(&pFid, &catPage, PAGE_BUF);
    if(e < 0) ERR(e);
    GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);
    fid = catEntry->fid;

    // 파라미터로 주어진 nearObj가 NULL이 아닌 경우,
    if (nearObj != NULL) {
        // 2. nearObj가 존재하는 Page (여유가 안될 경우 이 페이지에 들어가지 못할 수도 있다)
        MAKE_PAGEID(nearPid, nearObj->volNo, nearObj->pageNo);
        e = BfM_GetTrain(&nearPid, &apage, PAGE_BUF);
        if(e < 0) ERRB1(e, &pFid, PAGE_BUF); // 위에서 fix한 buffer를 unfix 한다.

        // nearObj가 저장된 page에 여유 공간이 있는 경우,
        if (neededSpace <= SP_FREE(apage)) {
            // 해당 page를 object를 삽입할 page로 선정함
            pid = nearPid;
            needToAllocPage = FALSE;

            // 선정된 page를 현재 available space list에서 삭제함
            om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);

            // 필요 시 선정된 page를 compact 함
            if (neededSpace > SP_CFREE(apage)) {
                // slotNo가 NIL (-1) 인 경우 -> Page의 모든 object들을 데이터 영역의 가장 앞부분부터 연속되게 저장
                e = EduOM_CompactPage(apage, NIL);
                if (e < 0) ERR1(e, &pid, PAGE_BUF);
            }

        }
        // nearObj가 저장된 page에 여유 공간이 없는 경우,
        else {
            // 새로운 page를 할당 받아 object를 삽입할 page로 선정함
            // 새롭게 page를 파일에 추가하므로, dirty bit를 set 한다.
            e = BfM_SetDirty(catObjForFile, PAGE_BUF);
            if (e < 0) ERR(e);

            // RDsM_AllocTrains()에 필요한 인자를 위해 firstExt 값 가져온다.
            e = RDsM_PageIdToExtNo(&pFid, &firstExt);
            if (e < 0) ERR(e);
            
            e = RDsM_AllocTrains(fid.volNo, firstExt, &nearPid, catEntry->eff, 1, 1, &pid);
            if (e < 0) ERR(e);
            e = BfM_GetNewTrain(&pid, &apage, PAGE_BUF);
            if (e < 0) ERR(e);

            // 선정된 page의 header를 초기화함
            apage->header.pid = pid;
            SET_PAGE_TYPE(apage, SLOTTED_PAGE_TYPE);
            apage->header.reserved = NIL;
            apage->header.nSlots = 0; // 사용중인 slot들 중 마지막 slot의 번호 + 1
            apage->header.free = 0;
            apage->header.unused = 0;
            apage->header.fid = fid;
            apage->header.unique = 0;
            apage->header.uniqueLimit = 0;
            
            // 선정된 page를 file 구성 page들로 이루어진 list에서 nearObj가 저장된 page의 다음 page로 삽입함
            e = om_FileMapAddPage(catObjForFile, &nearPid, &pid);
            if(e) ERRB1(e, &pid, PAGE_BUF);
        }   

    }
    // 파라미터로 주어진 nearObj가 NULL인 경우,
    else {
        // Object 삽입을 위해 필요한 자유 공간의 크기에 알맞은 available space list가 존재하는 경우,


        // Object 삽입을 위해 필요한 자유 공간의 크기에 알맞은 available space list가 존재하지 않고, 
        // file의 마지막 page에 여유 공간이 있는 경우,

        // file의 마지막 page에 여유 공간이 없는 경우,

    }

    // 선정된 page에 object를 삽입함


    // 삽입된 object의 ID를 반환함
    
    return(eNOERROR);
    
} /* eduom_CreateObject() */
