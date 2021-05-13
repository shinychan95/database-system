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
 * Module: edubtm_Split.c
 *
 * Description : 
 *  This file has three functions about 'split'.
 *  'edubtm_SplitInternal(...) and edubtm_SplitLeaf(...) insert the given item
 *  after spliting, and return 'ritem' which should be inserted into the
 *  parent page.
 *
 * Exports:
 *  Four edubtm_SplitInternal(ObjectID*, BtreeInternal*, Two, InternalItem*, InternalItem*)
 *  Four edubtm_SplitLeaf(ObjectID*, PageID*, BtreeLeaf*, Two, LeafItem*, InternalItem*)
 */


#include <string.h>
#include "EduBtM_common.h"
#include "BfM.h"
#include "EduBtM_Internal.h"



/*@================================
 * edubtm_SplitInternal()
 *================================*/
/*
 * Function: Four edubtm_SplitInternal(ObjectID*, BtreeInternal*,Two, InternalItem*, InternalItem*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  At first, the function edubtm_SplitInternal(...) allocates a new internal page
 *  and initialize it.  Secondly, all items in the given page and the given
 *  'item' are divided by halves and stored to the two pages.  By spliting,
 *  the new internal item should be inserted into their parent and the item will
 *  be returned by 'ritem'.
 *
 *  A temporary page is used because it is difficult to use the given page
 *  directly and the temporary page will be copied to the given page later.
 *
 * Returns:
 *  error code
 *    some errors caused by function calls
 *
 * Note:
 *  The caller should call BfM_SetDirty() for 'fpage'.
 * 
 */
Four edubtm_SplitInternal(
    ObjectID                    *catObjForFile,         /* IN catalog object of B+ tree file */
    BtreeInternal               *fpage,                 /* INOUT the page which will be splitted */
    Two                         high,                   /* IN slot No. for the given 'item' */
    InternalItem                *item,                  /* IN the item which will be inserted */
    InternalItem                *ritem)                 /* OUT the item which will be returned by spliting */
{
    Four                        e;                      /* error number */
    Two                         i;                      /* slot No. in the given page, fpage */
    Two                         j;                      /* slot No. in the splitted pages */
    Two                         k;                      /* slot No. in the new page */
    Two                         maxLoop;                /* # of max loops; # of slots in fpage + 1 */
    Four                        sum;                    /* the size of a filled area */
    Boolean                     flag = FALSE;           /* TRUE if 'item' become a member of fpage */
    PageID                      newPid;                 /* for a New Allocated Page */
    BtreeInternal               *npage;                 /* a page pointer for the new allocated page */
    Two                         fEntryOffset;           /* starting offset of an entry in fpage */
    Two                         nEntryOffset;           /* starting offset of an entry in npage */
    Two                         entryLen;               /* length of an entry */
    btm_InternalEntry           *fEntry;                /* internal entry in the given page, fpage */
    btm_InternalEntry           *nEntry;                /* internal entry in the new page, npage*/
    Boolean                     isTmp;


    
    return(eNOERROR);
    
} /* edubtm_SplitInternal() */



/*@================================
 * edubtm_SplitLeaf()
 *================================*/
/*
 * Function: Four edubtm_SplitLeaf(ObjectID*, PageID*, BtreeLeaf*, Two, LeafItem*, InternalItem*)
 *
 * Description: 
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  The function edubtm_SplitLeaf(...) is similar to edubtm_SplitInternal(...) except
 *  that the entry of a leaf differs from the entry of an internal and the first
 *  key value of a new page is used to make an internal item of their parent.
 *  Internal pages do not maintain the linked list, but leaves do it, so links
 *  are properly updated.
 *
 * Returns:
 *  Error code
 *  eDUPLICATEDOBJECTID_BTM
 *    some errors caused by function calls
 *
 * Note:
 *  The caller should call BfM_SetDirty() for 'fpage'.
 * 
 * 한글 설명:
 *  Overflow가 발생한 leaf page를 split 하여 파라미터로 주어진 index entry를 삽입하고, 
 *  split으로 생성된 새로운 leaf page를 가리키는 internal index entry를 반환함
 * 
 * 관련 함수:
 *  - edubtm_InitLeaf(),
 *  - edubtm_CompactLeafPage(), 
 *  - btm_AllocPage(), 
 *  - BfM_GetTrain(), 
 *  - BfM_GetNewTrain(), 
 *  - BfM_FreeTrain(), 
 *  - BfM_SetDirty()
 */
Four edubtm_SplitLeaf(
    ObjectID                    *catObjForFile, /* IN catalog object of B+ tree file */
    PageID                      *root,          /* IN PageID for the given page, 'fpage' */
    BtreeLeaf                   *fpage,         /* INOUT the page which will be splitted */
    Two                         high,           /* IN slotNo for the given 'item' */
    LeafItem                    *item,          /* IN the item which will be inserted */
    InternalItem                *ritem)         /* OUT the item which will be returned by spliting */
{
    Four                        e;              /* error number */
    Two                         i;              /* slot No. in the given page, fpage */
    Two                         j;              /* slot No. in the splitted pages */
    Two                         k;              /* slot No. in the new page */
    Two                         maxLoop;        /* # of max loops; # of slots in fpage + 1 */
    Four                        sum;            /* the size of a filled area */
    PageID                      newPid;         /* for a New Allocated Page */
    PageID                      nextPid;        /* for maintaining doubly linked list */
    BtreeLeaf                   tpage;          /* a temporary page for the given page */
    BtreeLeaf                   *npage;         /* a page pointer for the new page */
    BtreeLeaf                   *mpage;         /* for doubly linked list */
    btm_LeafEntry               *itemEntry;     /* entry for the given 'item' */
    btm_LeafEntry               *fEntry;        /* an entry in the given page, 'fpage' */
    btm_LeafEntry               *nEntry;        /* an entry in the new page, 'npage' */
    ObjectID                    *iOidArray;     /* ObjectID array of 'itemEntry' */
    ObjectID                    *fOidArray;     /* ObjectID array of 'fEntry' */
    Two                         fEntryOffset;   /* starting offset of 'fEntry' */
    Two                         nEntryOffset;   /* starting offset of 'nEntry' */
    Two                         oidArrayNo;     /* element No in an ObjectID array */
    Two                         alignedKlen;    /* aligned length of the key length */
    Two                         itemEntryLen;   /* length of entry for item */
    Two                         entryLen;       /* entry length */
    Boolean                     flag;
    Boolean                     isTmp;
 
    
    // 새로운 page를 할당 받음
    e = btm_AllocPage(catObjForFile, root, &newPid);
    if (e < eNOERROR) ERR(e);

    // 할당 받은 page를 leaf page로 초기화함
    e = edubtm_InitLeaf(&newPid, FALSE, FALSE);
    if (e < eNOERROR) ERR(e);

    // Disk 상에서 할당되지 않은 새로운 page/train을 buffer element에 fix 함
    e = BfM_GetNewTrain(&newPid, (char**)&npage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    // 기존 index entry들 및 삽입할 index entry를 key 순으로 정렬하여 overflow가 발생한 page 및 할당 받은 page에 나누어 저장함
    // 먼저, overflow가 발생한 page에 데이터 영역을 50% 이상 채우는 수의 index entry들을 저장함
    tpage = *fpage;
    
    // fpage에 다시 데이터를 입력한다.
    sum = 0;
    i = 0; // for fpage

    for (j = 0; j < tpage.hdr.nSlots && sum < BL_HALF; j++) {
        if (high == j) {
            alignedKlen = ALIGNED_LENGTH(item->klen);
            entryLen = sizeof(Two) + sizeof(Two) + alignedKlen + sizeof(ObjectID);

            fpage->slot[-i] = sum;
            fEntry = (btm_LeafEntry*)&fpage->data[fpage->slot[-i]];
            
            memcpy(fEntry, &item->nObjects, entryLen - OBJECTID_SIZE);
            memcpy(&fEntry->kval[alignedKlen], &item->oid, OBJECTID_SIZE);

            sum += entryLen;
            i++;

            if (sum < BL_HALF) break;
        }

        // j번째 슬롯에 해당하는 index entry들을 fpage에 새롭게 저장한다
        itemEntry = (btm_LeafEntry*)&tpage.data[tpage.slot[-j]];
        alignedKlen = ALIGNED_LENGTH(itemEntry->klen);
        entryLen = sizeof(Two) + sizeof(Two) + alignedKlen + sizeof(ObjectID);

        fpage->slot[-i] = sum;
        fEntry = (btm_LeafEntry*)&fpage->data[fpage->slot[-i]];

        memcpy(fEntry, itemEntry, entryLen);

        sum += entryLen;
        i++;
    }

    // fpage의 header를 갱신함
    fpage->hdr.free = sum;
    fpage->hdr.nSlots = i;
    fpage->hdr.unused = 0;

    fEntry = (btm_LeafEntry*)&fpage->data[fpage->slot[-i]];
    ritem->spid = newPid.pageNo;
    ritem->klen = fEntry->klen;
    memcpy(ritem->kval, fEntry->kval, fEntry->klen);


    // 나머지 index entry들을 할당 받은 npage에 저장함
    sum = 0;
    i = 0; // for fpage

    for (; j < tpage.hdr.nSlots; j++) {
        if (high == j) {
            alignedKlen = ALIGNED_LENGTH(item->klen);
            entryLen = sizeof(Two) + sizeof(Two) + alignedKlen + sizeof(ObjectID);

            npage->slot[-i] = sum;
            nEntry = (btm_LeafEntry*)&npage->data[npage->slot[-i]];
            
            memcpy(nEntry, &item->nObjects, entryLen - OBJECTID_SIZE);
            memcpy(&nEntry->kval[alignedKlen], &item->oid, OBJECTID_SIZE);

            sum += entryLen;
            i++;
        }

        // j번째 슬롯에 해당하는 index entry들을 npage에 새롭게 저장한다
        itemEntry = (btm_LeafEntry*)&tpage.data[tpage.slot[-j]];
        alignedKlen = ALIGNED_LENGTH(itemEntry->klen);
        entryLen = sizeof(Two) + sizeof(Two) + alignedKlen + sizeof(ObjectID);

        npage->slot[-i] = sum;
        nEntry = (btm_LeafEntry*)&npage->data[npage->slot[-i]];

        memcpy(nEntry, itemEntry, entryLen);
        
        sum += entryLen;
        i++;
    }

    // npage의 header를 갱신함
    npage->hdr.free = sum;
    npage->hdr.nSlots = i;
    npage->hdr.unused = 0;


    // 할당 받은 page를 leaf page들간의 doubly linked list에 추가함
    npage->hdr.nextPage = fpage->hdr.nextPage;
    npage->hdr.prevPage = fpage->hdr.pid.pageNo;
    fpage->hdr.nextPage = newPid.pageNo;

    // 할당 받은 page를 가리키는 internal index entry를 생성함
    itemEntry = (btm_LeafEntry*)&npage->data[npage->slot[0]];
    ritem->spid = newPid.pageNo;
    ritem->klen = itemEntry->klen;
    memcpy(ritem->kval, itemEntry->kval, itemEntry->klen);


    e = BfM_SetDirty(&newPid, PAGE_BUF);
    if (e < eNOERROR) ERRB1(e, npage, PAGE_BUF);

    e = BfM_SetDirty(&fpage->hdr.pid, PAGE_BUF);
    if (e < eNOERROR) ERRB1(e, fpage, PAGE_BUF);
    
    e = BfM_FreeTrain(&newPid, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    return(eNOERROR);
    
} /* edubtm_SplitLeaf() */
