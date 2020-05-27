// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assert.h"

#include "chainparams.h"
#include "main.h"
#include "util.h"

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

// Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress> &vSeedsOut, const SeedSpec6 *data, unsigned int count)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7*24*60*60;
    for (unsigned int i = 0; i < count; i++)
    {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

//
// Main network
//
class CMainParams : public CChainParams {
public:
    CMainParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0x12;
        pchMessageStart[1] = 0xaa;
        pchMessageStart[2] = 0x9f;
        pchMessageStart[3] = 0xc3;
        vAlertPubKey = ParseHex("113fbb2ae311f123756b659c91a56897eba2efaacb6a192acdbef7894465f81f85d131aadfef3be6145678454852a2d08c6314bba5ca3cbe5616262da3b1a6fffa");
        nDefaultPort = 20995; // peer
        nRPCPort = 20925; // rpc
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 18);
        bnProofOfStakeLimit = CBigNum(~uint256(0) >> 18);

        const char* pszTimestamp = "Why Bitcoin Traders Should Care About Double-Digit Inflation in UK | Helene Braun | Aug 17, 2022";
        std::vector<CTxIn> vin;
        vin.resize(1);
        vin[0].scriptSig = CScript() << 0 << CBigNum(42) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        std::vector<CTxOut> vout;
        vout.resize(1);
        vout[0].nValue = 1 * COIN;
        vout[0].SetEmpty();
        CTransaction txNew(1, 1660778048, vin, vout, 0);
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = 1660778048;
        genesis.nBits    = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce   = 378454;

        /** Genesis Block MainNet */
        /*
        Hashed MainNet Genesis Block Output
        block.hashMerkleRoot == 5ea79d7abd27df58c93a325c34a01916fa38f891ed1049f51bceb47adb4595d1
        block.nTime = 1660778048
        block.nNonce = 378454
        block.GetHash = 00000ef6d869584ea6b9b3e6ecc39fa4bb41a0a916ed0914657c1a865f642229
        */

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x00000ef6d869584ea6b9b3e6ecc39fa4bb41a0a916ed0914657c1a865f642229"));
        assert(genesis.hashMerkleRoot == uint256("0x5ea79d7abd27df58c93a325c34a01916fa38f891ed1049f51bceb47adb4595d1"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,73);// W
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,58);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,55);
        base58Prefixes[STEALTH_ADDRESS] = std::vector<unsigned char>(1,23);
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        // vSeeds.push_back(CDNSSeedData("node0",  "000.000.000.000"));

        convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main));

        nEndPoWBlock = 0x7fffffff;
        nStartPoSBlock = 1;
    }

    virtual const CBlock& GenesisBlock() const { return genesis; }
    virtual Network NetworkID() const { return CChainParams::MAIN; }

    virtual const vector<CAddress>& FixedSeeds() const {
        return vFixedSeeds;
    }
protected:
    CBlock genesis;
    vector<CAddress> vFixedSeeds;
};
static CMainParams mainParams;


//
// Testnet
//
class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0x99;
        pchMessageStart[1] = 0xff;
        pchMessageStart[2] = 0xcc;
        pchMessageStart[3] = 0xed;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 15);
        bnProofOfStakeLimit = CBigNum(~uint256(0) >> 15);
        vAlertPubKey = ParseHex("113fbb2ae311f123756b659c91a56897eba2efaacb6a192acdbef7894465f81f85d131aadfef3be6145678454852a2d08c6314bba5ca3cbe5616262da3b1a6fffb");
        nDefaultPort = 20099; // 
        nRPCPort = 20198; //
        strDataDir = "testnet";

        // Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime  = 1660778088;
        genesis.nBits  = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce = 38688;

        /** Genesis Block TestNet */
        /*
        Hashed TestNet Genesis Block Output
        block.hashMerkleRoot == 5ea79d7abd27df58c93a325c34a01916fa38f891ed1049f51bceb47adb4595d1
        block.nTime = 1660778088
        block.nNonce = 38688
        block.GetHash = 0000189c2b73e05d130266806eb85548cbaf34410f9bf24cf0ec3417efd8f112
        */

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x0000189c2b73e05d130266806eb85548cbaf34410f9bf24cf0ec3417efd8f112"));

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,74); // W or X
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,44); //
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,32); //
        base58Prefixes[STEALTH_ADDRESS] = std::vector<unsigned char>(1,57); //
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        nEndPoWBlock = 0x7fffffff;
    }
    virtual Network NetworkID() const { return CChainParams::TESTNET; }
};
static CTestNetParams testNetParams;

//
// Regression test
//
class CRegTestParams : public CTestNetParams {
public:
    CRegTestParams() {
        pchMessageStart[0] = 0xa8;
        pchMessageStart[1] = 0xf8;
        pchMessageStart[2] = 0x8c;
        pchMessageStart[3] = 0xff;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 1);
        genesis.nTime = 1660778098;
        genesis.nBits  = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce = 8;
        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 20009; //
        strDataDir = "regtest";

        /** Genesis Block RegNet */
        /*
        Hashed RegNet Genesis Block Output
        block.hashMerkleRoot == 5ea79d7abd27df58c93a325c34a01916fa38f891ed1049f51bceb47adb4595d1
        block.nTime = 1660778098
        block.nNonce = 8
        block.GetHash = 23a85f5a5c7a0ce6201e3a21de44d98874c8379be42c4d4fc918f51ea86133c1
        */

        assert(hashGenesisBlock == uint256("0x23a85f5a5c7a0ce6201e3a21de44d98874c8379be42c4d4fc918f51ea86133c1"));

        vSeeds.clear();  // Regtest mode doesn't have any DNS seeds.
    }

    virtual bool RequireRPCPassword() const { return false; }
    virtual Network NetworkID() const { return CChainParams::REGTEST; }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = &mainParams;

const CChainParams &Params() {
    return *pCurrentParams;
}

void SelectParams(CChainParams::Network network) {
    switch (network) {
        case CChainParams::MAIN:
            pCurrentParams = &mainParams;
            break;
        case CChainParams::TESTNET:
            pCurrentParams = &testNetParams;
            break;
        case CChainParams::REGTEST:
            pCurrentParams = &regTestParams;
            break;
        default:
            assert(false && "Unimplemented network");
            return;
    }
}

bool SelectParamsFromCommandLine() {
    bool fRegTest = GetBoolArg("-regtest", false);
    bool fTestNet = GetBoolArg("-testnet", false);

    if (fTestNet && fRegTest) {
        return false;
    }

  