
// Copyright (c) 2016-2020 The CryptoCoderz Team / Espers
// Copyright (c) 2018-2020 The CryptoCoderz Team / INSaNe project
// Copyright (c) 2018-2020 The Rubix project
// Copyright (c) 2022 The WayaWolfCoin project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_BLOCKPARAMS_H
#define BITCOIN_BLOCKPARAMS_H

#include "net.h"
#include "chain.h"
#include "bignum.h"
#include "base58.h"

// Define difficulty retarget algorithms
enum DiffMode {
    DIFF_DEFAULT = 0, // Default to invalid 0
    DIFF_VRX     = 1, // Retarget using Terminal-Velocity-RateX
};

void VRXswngdebug(bool fProofOfStake);
void VRXdebug();
void GNTdebug();
void VRX_BaseEngine(const CBlockIndex* pindexLast, bool fProofOfStake);
void VRX_Simulate_Retarget();
double VRX_GetPrevDiff(bool fPoS);
void VRX_ThreadCurve(const CBlockIndex* pindexLast, bool fProofOfStake);
void VRX_Dry_Run(const CBlockIndex* pindexLast);
unsigned int VRX_Retarget(const CBlockIndex* pindexLast, bool fProofOfStake);
unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast, bool fProofOfStake);
bool fMNtier2();
int64_t GetProofOfWorkReward(int nHeight, int64_t nFees);
int64_t GetProofOfStakeReward(const CBlockIndex* pindexPrev, int64_t nCoinAge, int64_t nFees);

#endif // BITCOIN_BLOCKPARAMS_H