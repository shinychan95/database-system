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
 * Module: edubfm_AllocTrain.c
 *
 * Description : 
 *  Allocate a new buffer from the buffer pool.
 *
 * Exports:
 *  Four edubfm_AllocTrain(Four)
 */


#include <errno.h>
#include "EduBfM_common.h"
#include "EduBfM_Internal.h"


extern CfgParams_T sm_cfgParams;

/*@================================
 * edubfm_AllocTrain()
 *================================*/
/*
 * Function: Four edubfm_AllocTrain(Four)
 *
 * Description : 
 * (Following description is for original ODYSSEUS/COSMOS BfM.
 *  For ODYSSEUS/EduCOSMOS EduBfM, refer to the EduBfM project manual.)
 *
 *  Allocate a new buffer from the buffer pool.
 *  The used buffer pool is specified by the parameter 'type'.
 *  This routine uses the second chance buffer replacement algorithm
 *  to select a victim.  That is, if the reference bit of current checking
 *  entry (indicated by BI_NEXTVICTIM(type), macro for
 *  bufInfo[type].nextVictim) is set, then simply clear
 *  the bit for the second chance and proceed to the next entry, otherwise
 *  the current buffer indicated by BI_NEXTVICTIM(type) is selected to be
 *  returned.
 *  Before return the buffer, if the dirty bit of the victim is set, it 
 *  must be force out to the disk.
 *
 * Returns;
 *  1) An index of a new buffer from the buffer pool
 *  2) Error codes: Negative value means error code.
 *     eNOUNFIXEDBUF_BFM - There is no unfixed buffer.
 *     some errors caused by fuction calls
 * 
 * 설명:
 *  bufferPool에서 page/train을 저장하기 위한 buffer element를 한 개 할당 받고, 
 *  해당 buffer element의 array index를 반환함
 * 
 * 관련 함수:
 *  1. edubfm_Delete() - hashTable에서 buffer element의 array index를 삭제함
 *  2. edubfm_FlushTrain() - 수정된 page/train을 disk에 기록함
 */
Four edubfm_AllocTrain(
    Four 	type)			/* IN type of buffer (PAGE or TRAIN) */
{
    Four 	e;			    /* for error */
    Four 	victim;			/* return value */
    Four 	i;
    

	/* Error check whether using not supported functionality by EduBfM */
	if(sm_cfgParams.useBulkFlush) ERR(eNOTSUPPORTED_EDUBFM);

    victim = BI_NEXTVICTIM(type);

    // Second chance buffer replacement algorithm을 사용하여, 할당 받을 buffer element를 선정함
    // 할당 대상 선정을 위해 대응하는 fixed 변수 값이 0인 buffer element들을 순차적으로 방문함
    for (i = 0; i < BI_NBUFS(type) * 2; i++) {
        if (BI_FIXED(type, victim) == 0) {
            if (BI_BITS(type, victim) & REFER) {
                BI_BITS(type, victim) ^= REFER;
            }
            else {
                break;
            }
        }

        victim = (victim + 1) % BI_NBUFS(type);
    }

    // 선정된 victim이 없을 경우 에러를 반환
    if (i == BI_NBUFS(type) * 2) ERR(eNOUNFIXEDBUF_BFM);

    // 선정된 buffer element와 관련된 데이터 구조를 초기화함
    // 선정된 buffer element에 저장되어 있던 page/train이 수정된 경우, 기존 buffer element의 내용을 disk로 flush함
    if (BI_BITS(type, victim) & DIRTY) {
        e = edubfm_FlushTrain(&BI_KEY(type, victim), type);
        if (e < 0) ERR(e);
    }

    // 선정된 buffer element에 대응하는 bufTable element를 초기화함
    // 질문? - EduBfM_GetTrain에서 초기화하지 않나?

    // 선정된 buffer element의 array index (hashTable entry) 를 hashTable에서 삭제함
    if (BI_KEY(type, victim).pageNo != NIL) {
        e = edubfm_Delete(&BI_KEY(type, victim), type);
        if (e < 0) ERR(e);
    }

    // 선정된 buffer element의 array index를 반환함
    return( victim );
    
}  /* edubfm_AllocTrain */
