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
 * Module: edubfm_FlushTrain.c
 *
 * Description : 
 *  Write a train specified by 'trainId' into the disk.
 *
 * Exports:
 *  Four edubfm_FlushTrain(TrainID *, Four)
 */


#include "EduBfM_common.h"
#include "RDsM.h"
#include "RM.h"
#include "EduBfM_Internal.h"



/*@================================
 * edubfm_FlushTrain()
 *================================*/
/*
 * Function: Four edubfm_FlushTrain(TrainID*, Four)
 *
 * Description : 
 * (Following description is for original ODYSSEUS/COSMOS BfM.
 *  For ODYSSEUS/EduCOSMOS EduBfM, refer to the EduBfM project manual.)
 *
 *  Write a train specified by 'trainId' into the disk.
 *  Construct a hash key using the TrainID 'trainId'(actually same)
 *  in order to look up the buffer in the buffer pool. If it is successfully
 *  found, then force it out to the disk using RDsM, especially
 *  RDsM_WriteTrain().
 *
 * Returns:
 *  error code
 *    some errors caused by function calls
 * 
 * 설명:
 *  수정된 page/train을 disk에 기록함
 * 
 * 관련 함수:
 *  1. edubfm_LookUp()
 *  2. RDsM_WriteTrain()
 */
Four edubfm_FlushTrain(
    TrainID 			*trainId,		/* IN train to be flushed */
    Four   			type)			/* IN buffer type */
{
    Four 			e;			/* for errors */
    Four 			index;			/* for an index */

	/* Error check whether using not supported functionality by EduBfM */
	if (RM_IS_ROLLBACK_REQUIRED()) ERR(eNOTSUPPORTED_EDUBFM);

    /* Is the buffer type valid? */
    if(IS_BAD_BUFFERTYPE(type)) ERR(eBADBUFFERTYPE_BFM);	

    // Flush 할 page/train의 hash key value를 이용하여, 
    // 해당 page/train이 저장된 buffer element의 array index를 hashTable에서 검색함
	index = edubfm_LookUp((BfMHashKey *)trainId, type);
    if (index < 0) ERR(eBADHASHKEY_BFM);

    // 해당 buffer element에 대한 DIRTY bit가 1로 set 된 경우, 해당 page/train을 disk에 기록함
    if (BI_BITS(type, index) & DIRTY) {
        e = RDsM_WriteTrain(BI_BUFFER(type, index), (PageID *) trainId, BI_BUFSIZE(type));
        if(e < 0) ERR(e);

        // 해당 DIRTY bit를 unset 함
        BI_BITS(type, index) ^= DIRTY;
    }

    return( eNOERROR );

}  /* edubfm_FlushTrain */
