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
 * Module: edubfm_Hash.c
 *
 * Description:
 *  Some functions are provided to support buffer manager.
 *  Each BfMHashKey is mapping to one table entry in a hash table(hTable),
 *  and each entry has an index which indicates a buffer in a buffer pool.
 *  An ordinary hashing method is used and linear probing strategy is
 *  used if collision has occurred.
 *
 * Exports:
 *  Four edubfm_LookUp(BfMHashKey *, Four)
 *  Four edubfm_Insert(BfMHaskKey *, Two, Four)
 *  Four edubfm_Delete(BfMHashKey *, Four)
 *  Four edubfm_DeleteAll(void)
 */


#include <stdlib.h> /* for malloc & free */
#include "EduBfM_common.h"
#include "EduBfM_Internal.h"



/*@
 * macro definitions
 */  

/* Macro: BFM_HASH(k,type)
 * Description: return the hash value of the key given as a parameter
 * Parameters:
 *  BfMHashKey *k   : pointer to the key
 *  Four type       : buffer type
 * Returns: (Two) hash value
 */
#define BFM_HASH(k,type)	(((k)->volNo + (k)->pageNo) % HASHTABLESIZE(type))


/*@================================
 * edubfm_Insert()
 *================================*/
/*
 * Function: Four edubfm_Insert(BfMHashKey *, Two, Four)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BfM.
 *  For ODYSSEUS/EduCOSMOS EduBfM, refer to the EduBfM project manual.)
 *
 *  Insert a new entry into the hash table.
 *  If collision occurs, then use the linear probing method.
 *
 * Returns:
 *  error code
 *    eBADBUFINDEX_BFM - bad index value for buffer table
 * 
 * 설명:
 *  hashTable에 buffer element의 array index를 삽입함
 * 
 */
Four edubfm_Insert(
    BfMHashKey 		*key,			/* IN a hash key in Buffer Manager */
    Two 		index,			/* IN an index used in the buffer pool */
    Four 		type)			/* IN buffer type */
{
    Four 		i;			
    Two  		hashValue;


    CHECKKEY(key);    /*@ check validity of key */

    if(index < 0 || index > BI_NBUFS(type)) ERR(eBADBUFINDEX_BFM);

    // 해당 buffer element에 저장된 page/train의 hash key value를 이용하여, 
    // hashTable에서 해당 array index를 삽입할 위치를 결정함

    hashValue = BFM_HASH(key, type);
    i = BI_HASHTABLEENTRY(type, hashValue);

    // Collision이 발생하지 않은 경우, 해당 array index를 결정된 위치에 삽입함
    if (i == NIL) {
        BI_HASHTABLEENTRY(type, hashValue) = index;
    }
    // Collision이 발생한 경우, chaining 방법을 사용하여 이를 처리함
    else {
        BI_NEXTHASHENTRY(type, index) = i;
        BI_HASHTABLEENTRY(type, hashValue) = index;
    }
   

    return( eNOERROR );

}  /* edubfm_Insert */



/*@================================
 * edubfm_Delete()
 *================================*/
/*
 * Function: Four edubfm_Delete(BfMHashKey *, Four)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BfM.
 *  For ODYSSEUS/EduCOSMOS EduBfM, refer to the EduBfM project manual.)
 *
 *  Look up the entry which corresponds to `key' and
 *  Delete the entry from the hash table.
 *
 * Returns:
 *  error code
 *    eNOTFOUND_BFM - The key isn't in the hash table.
 * 
 * 설명:
 *  hashTable에서 buffer element의 array index를 삭제함
 */
Four edubfm_Delete(
    BfMHashKey          *key,                   /* IN a hash key in buffer manager */
    Four                type )                  /* IN buffer type */
{
    Two                 i, prev;                
    Two                 hashValue;

    CHECKKEY(key);    /*@ check validity of key */

    // 해당 buffer element에 저장된 page/train의 hash key value를 이용하여, 
    // 삭제할 buffer element의 array index를 hashTable에서 검색함
    hashValue = BFM_HASH(key, type);

    prev = NIL;
    i = BI_HASHTABLEENTRY(type, hashValue);

    while (i != NIL) {
        // 검색된 entry (array index) 를 hashTable에서 삭제함
        if (EQUALKEY(&BI_KEY(type, i), key)) {
            // prev가 없는 경우 = hashTable 내 entry 존재하는 경우
            if (prev == NIL) {
                if (BI_NEXTHASHENTRY(type, i) == NIL) {
                    BI_HASHTABLEENTRY(type, hashValue) = NIL;
                }
                else {
                    BI_HASHTABLEENTRY(type, hashValue) = BI_NEXTHASHENTRY(type, i);
                }
            }
            // prev가 있고, nextHashEntry가 없는 경우
            else if (BI_NEXTHASHENTRY(type, i) == NIL) {
                BI_NEXTHASHENTRY(type, prev) = NIL;
            }
            // prev가 있고, nextHashEntry가 있는 경우
            else {
                BI_NEXTHASHENTRY(type, prev) = BI_NEXTHASHENTRY(type, i);
            }
            
            return (eNOERROR);
        }
        
        prev = i;
        i = BI_NEXTHASHENTRY(type, i);
    }

    ERR( eNOTFOUND_BFM );

}  /* edubfm_Delete */



/*@================================
 * edubfm_LookUp()
 *================================*/
/*
 * Function: Four edubfm_LookUp(BfMHashKey *, Four)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BfM.
 *  For ODYSSEUS/EduCOSMOS EduBfM, refer to the EduBfM project manual.)
 *
 *  Look up the given key in the hash table and return its
 *  corressponding index to the buffer table.
 *
 * Retruns:
 *  index on buffer table entry holding the train specified by 'key'
 *  (NOTFOUND_IN_HTABLE - The key don't exist in the hash table.)
 * 
 * 설명:
 *  hashTable에서 파라미터로 주어진 hash key (BfMHashKey) 에 대응하는
 *  buffer element의 array index를 검색하여 반환함
 */
Four edubfm_LookUp(
    BfMHashKey          *key,                   /* IN a hash key in Buffer Manager */
    Four                type)                   /* IN buffer type */
{
    Two                 i, j;                   /* indices */
    Two                 hashValue;

    CHECKKEY(key);    /*@ check validity of key */

    hashValue = BFM_HASH(key, type);

    // 해당 hash key를 갖는 page/train이 저장된 buffer element의 array index를 hashTable에서 검색함
    // hashValue가 같더라도 volNo와 pageNo가 다를 수 있다.
    i = BI_HASHTABLEENTRY(type, hashValue);
    
    while (i != NIL) {
        // 검색된 array index를 반환함
        if (EQUALKEY(&BI_KEY(type, i), key)) return i;
        
        // 혹시나 hashValue가 같은 next 값이 있다면 똑같이 비교
        i = BI_NEXTHASHENTRY(type, i);
    }
    
    return(NOTFOUND_IN_HTABLE);
    
}  /* edubfm_LookUp */



/*@================================
 * edubfm_DeleteAll()
 *================================*/
/*
 * Function: Four edubfm_DeleteAll(void)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BfM.
 *  For ODYSSEUS/EduCOSMOS EduBfM, refer to the EduBfM project manual.)
 *
 *  Delete all hash entries.
 *
 * Returns:
 *  error code
 * 
 * 설명:
 *  각 hashTable에서 모든 entry (buffer element의 array index) 들을 삭제함
 */
Four edubfm_DeleteAll(void)
{
    Two 	    i;
    Four        type;
    Four        tableSize;
    
    for (type = 0; type < NUM_BUF_TYPES; type++) {
        tableSize = HASHTABLESIZE(type);

        // chaining 까지 삭제하지는 않는다. 왜냐하면 bufTable도 EduBfM_DiscardAll() 함수에서 모두 지워지기 때문에 
        for (i = 0; i < tableSize; i++) {
            BI_HASHTABLEENTRY(type, i) = NIL;
        }
    }

    return(eNOERROR);

} /* edubfm_DeleteAll() */ 
