// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "hmac_sha256.h"

#include <string.h>

CHMAC_SHA256::CHMAC_SHA256(const unsigned char* key, size_t keylen)
{
    unsigned char rkey[64];
    if (keylen <= 64) {
        memcpy(rkey, key, keylen);
        memset(rkey + keylen, 0, 64 - keylen);
    } else {
        CSHA256().Write(key, keylen).Finalize(rkey)