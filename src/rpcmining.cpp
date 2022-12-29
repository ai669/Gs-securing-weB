// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcserver.h"
#include "blockparams.h"
#include "chainparams.h"
#include "main.h"
#include "db.h"
#include "txdb.h"
#include "init.h"
#include "miner.h"
#include "kernel.h"

#include <boost/assign/list_of.hpp>

using namespace json_spirit;
using namespace std;
using namespace boost::assign;

// Key used by getwork/getblocktemplate miners.
// Allocated in InitRPCMining, free'd in ShutdownRPCMining
static CReserveKey* pMiningKey = NULL;

void InitRPCMining()
{
    if (!pwalletMain)
        return;

    // getwork/getblocktemplate mining rewards paid here:
    pMiningKey = new CReserveKey(pwalletMain);
}

void ShutdownRPCMining()
{
    if (!pMiningKey)
        return;

    delete pMiningKey; pMiningKey = NULL;
}

Value getsubsidy(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getsubsidy [nTarget]\n"
            "Returns proof-of-work subsidy value for the specified value of target.");

    return (int64_t)GetProofOfStakeReward(pindexBest->pprev, 0, 0);
}

Value getstakesubsidy(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getstakesubsidy <hex string>\n"
            "Returns proof-of-stake subsidy value for the specified coinstake.");

    RPCTypeCheck(params, list_of(str_type));

    vector<unsigned char> txData(ParseHex(params[0].get_str()));
    CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
    CTransaction tx;
    try {
        ssData >> tx;
    }
    catch (std::exception &e) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }

    uint64_t nCoinAge;
    CTxDB txdb("r");
    if (!tx.GetCoinAge(txdb, pindexBest, nCoinAge))
        throw JSONRPCError(RPC_MISC_ERROR, "GetCoinAge failed");

    return (uint64_t)GetProofOfStakeReward(pindexBest->pprev, nCoinAge, 0);
}

Value getmininginfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getmininginfo\n"
            "Returns an object containing mining-related information.");

    uint64_t nWeight = 0;
    if (pwalletMain)
        nWeight = pwalletMain->GetStakeWeight();

    // Define block rewards
    int64_t nRewardPoW = (uint64_t)GetProofOfWorkReward(nBestHeight, 0);

    Object obj, diff, weight;
    obj.push_back(Pair("blocks",        (int)nBestHeight));
    obj.push_back(Pair("currentblocksize",(uint64_t)nLastBlockSize));
    obj.push_back(Pair("currentblocktx",(uint64_t)nLastBlockTx));

    diff.push_back(Pair("proof-of-work", GetDifficulty()));
    diff.push_back(Pair("proof-of-stake", GetDifficulty(GetLastBlockIndex(pindexBest, true))));
    diff.push_back(Pair("search-interval", (int)nLastCoinStakeSearchInterval));
    obj.push_back(Pair("difficulty", diff));

    obj.push_back(Pair("blockvalue-PoS", (uint64_t)getstakesubsidy));
    obj.push_back(Pair("blockvalue-PoW", nRewardPoW));
    obj.push_back(Pair("netmhashps",  GetPoWMHashPS()));
    obj.push_back(Pair("netstakeweight", GetPoSKernelPS()));
    obj.push_back(Pair("errors", GetWarnings("statusbar")));
    obj.push_back(Pair("pooledtx", (uint64_t)mempool.size()));

    weight.push_back(Pair("minimum", (uint64_t)nWeight));
    weight.push_back(Pair("maximum", (uint64_t)0));
    weight.push_back(Pair("combined", (uint64_t)nWeight));
    obj.push_back(Pair("stakeweight", weight));

    obj.push_back(Pair("stakeinterest", (uint64_t)getstakesubsidy));
    obj.push_back(Pair("testnet", TestNet()));
    return obj;
}

Value getstakinginfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getstakinginfo\n"
            "Returns an object containing staking-related information.");

    uint64_t nWeight = 0;
    uint64_t nExpectedTime = 0;

    if (pwalletMain)
        nWeight = pwalletMain->GetStakeWeight();

    uint64_t nNetworkWeight = GetPoSKernelPS();
    bool staking = nLastCoinStakeSearchInterval && nWeight;
    nExpectedTime = staking ? (GetTargetSpacing * nNetworkWeight / nWeight) : 0;

    Object obj;

    obj.push_back(Pair("enabled", GetBoolArg("-staking", true)));
    obj.push_back(Pair("staking", staking));
    obj.push_back(Pair("errors", GetWarnings("statusbar")));

    obj.push_back(Pair("currentblocksize", (uint64_t)nLastBlockSize));
    obj.push_back(Pair("currentblocktx", (uint64_t)nLastBlockTx));
    obj.push_back(Pair("pooledtx", (uint64_t)mempool.size()));

    obj.push_back(Pair("difficulty", GetDifficulty(GetLastBlockIndex(pindexBest, true))));
    obj.push_back(Pair("search-interval", (int)nLastCoinStakeSearchInterval));

    obj.push_back(Pair("weight", (uint64_t)nWeight));
    obj.push_back(Pair("netstakeweight", (uint64_t)nNetworkWeight));

    obj.push_back(Pair("expectedtime", nExpectedTime));

    obj.push_back(Pair("stakethreshold", GetStakeCombineThreshold() / COIN));

    return obj;
}

Value checkkernel(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2) {
        throw runtime_error(
            "checkkernel [{\"txid\":txid,\"vout\":n},...] [createblocktemplate=false]\n"
            "Check if one of given inputs is a kernel input at the moment.\n"
        );
    }

    RPCTypeCheck(params, list_of(array_type)(bool_type));

    Array inputs = params[0].get_array();
    bool fCreateBlockTemplate = params.size() > 1 ? params[1].get_bool() : false;

    if (vNodes.empty())
        throw JSONRPCError(-9, "WayaWolfCoin is not connected!");

    if (IsInitialBlockDownload())
        throw JSONRPCError(-10, "WayaWolfCoin is downloading blocks...");

    COutPoint kernel;
    CBlockIndex* pindexPrev = pindexBest;
    unsigned int nBits = GetNextTargetRequired(pindexPrev, true);
    int64_t nTime = GetAdjustedTime();
    nTime &= ~STAKE_TIMESTAMP_MASK;

    BOOST_FOREACH(Value& input, inputs)
    {
        const Object& o = input.get_obj();

        const Value& txid_v = find_value(o, "txid");
        if (txid_v.type() != str_type)
            throw JSONRPCError(RPC_INVALI