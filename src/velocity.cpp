// Copyright (c) 2014 The Cryptocoin WayaWolfCoinival Foundation
// Copyright (c) 2015-2020 The CryptoCoderz Team / Espers
// Copyright (c) 2018-2020 The Rubix Project
// Copyright (c) 2022 The WayaWolfCoin project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockparams.h"
#include "main.h"
#include "txdb.h"
#include "velocity.h"
#include "rpcserver.h"
#include "wallet.h"

bool VELOCITY_FACTOR = false;
uint256 RollingBlock;
int64_t RollingHeight;

/* VelocityI(int nHeight) ? i : -1
   Returns i or -1 if not found */
int VelocityI(int nHeight)
{
    if (FACTOR_TOGGLE(nHeight)) {
        VELOCITY_FACTOR = true;
    }
    return nHeight;
}

/* Velocity(int nHeight) ? true : false
   Returns true if nHeight is higher or equal to VELOCITY_HEIGHT */
bool Velocity_check(int nHeight)
{
    LogPrintf("Checking for Velocity on block %u: ",nHeight);
    if(VelocityI(nHeight) >= VELOCITY_HEIGHT[nHeight])
    {
        LogPrintf("Velocity is currently Enabled\n");
        return true;
    }
    LogPrintf("Velocity is currently disabled\n");
    return false;
}

/* Velocity(CBlockIndex* prevBlock, CBlock* block) ? true : false
   Goes close to the top of CBlock::AcceptBlock
   Returns true if proposed Block matches constrains */
bool Velocity(CBlockIndex* prevBlock, CBlock* block, bool fFactor_tx)
{
    // Define values
    int64_t TXrate = 0;
    int64_t CURvalstamp  = 0;
    int64_t OLDvalstamp  = 0;
    int64_t CURstamp = 0;
    int64_t OLDstamp = 0;
    int64_t TXstampC = 0;
    int64_t TXstampO = 0;
    int64_t SYScrntstamp = 0;
    int64_t SYSbaseStamp = 0;
    int nHeight = prevBlock->nHeight+1;
    int i = VelocityI(nHeight);
    // Set stanard values
    TXrate = block->GetBlockTime() - prevBlock->GetBlockTime();
    TXstampC = block->nTime;
    TXstampO = prevBlock->nTime;
    CURstamp = block->GetBlockTime();
    OLDstamp = prevBlock->GetBlockTime();
    CURvalstamp = prevBlock->GetBlockTime() + VELOCITY_MIN_RATE[i];
    OLDvalstamp = prevBlock->pprev->GetBlockTime() + VELOCITY_MIN_RATE[i];
    SYScrntstamp = GetAdjustedTime() + VELOCITY_MIN_RATE[i];
    SYSbaseStamp = GetTime() + VELOCITY_MIN_RATE[i];

    // Factor in T