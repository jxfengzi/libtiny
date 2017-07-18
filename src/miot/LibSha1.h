///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  LibSha1
//
//  Implementation of SHA1 hash function.
//  Original author:  Steve Reid <sreid@sea-to-sky.net>
//  Contributions by: James H. Brown <jbrown@burgoyne.com>, Saul Kravitz <Saul.Kravitz@celera.com>,
//  and Ralph Giles <giles@ghostscript.com>
//  Modified by WaterJuice retaining Public Domain license.
//
//  This is free and unencumbered software released into the public domain - June 2013 waterjuice.org
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _LibSha1_h_
#define _LibSha1_h_

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  IMPORTS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>

#include "tiny_base.h"


TINY_BEGIN_DECLS

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  TYPES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Sha1Context - This must be initialised using Sha1Initialised. Do not modify the contents of this structure directly.
typedef struct
{
    uint32_t        State[5];
    uint32_t        Count[2];
    uint8_t         Buffer[64];
} Sha1Context;

#define SHA1_HASH_SIZE           ( 160 / 8 )

typedef struct
{
    uint8_t      bytes [SHA1_HASH_SIZE];
} SHA1_HASH;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  PUBLIC FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Sha1Initialise
//
//  Initialises an SHA1 Context. Use this to initialise/reset a context.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
    Sha1Initialise
    (
        Sha1Context*                Context
    );

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Sha1Update
//
//  Adds data to the SHA1 context. This will process the data and update the internal state of the context. Keep on
//  calling this function until all the data has been added. Then call Sha1Finalise to calculate the hash.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
    Sha1Update
    (
        Sha1Context*        Context,
        void*               Buffer,
        uint32_t            BufferSize
    );

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Sha1Finalise
//
//  Performs the final calculation of the hash and returns the digest (20 byte buffer containing 160bit hash). After
//  calling this, Sha1Initialised must be used to reuse the context.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
    Sha1Finalise
    (
        Sha1Context*                Context,
        SHA1_HASH*                  Digest
    );

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TINY_END_DECLS

#endif //_LibSha1_h_

