#include <boost/test/unit_test.hpp>

#include "hash.h"
#include "util.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(hmac_tests)

typedef struct {
    const char *pszKey;
    const char *pszData;
    const char *pszMAC;
} testvec_t;

// test cases 1, 2, 3, 4, 6 and 7 of RFC 4231
static const testvec_t vtest[] = {
    {
        "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b"
        "0b0b0b0b",
        "4869205468657265",
        "87aa7cdea5ef619d4ff0b4241a1d6cb0"
        "2379f4e2ce4ec2787ad0b30545e17cde"
        "daa833b7d6b8a702038b274eaea3f4e4"
        "be9d914eeb61f1702e696c203a126854"
    },
    {
        "4a656665",
        "7768617420646f2079612077616e7420"
        "666f72206e6f7468696e673f",
        "164b7a7bfcf819e2e395fbe73b56e0a3"
        "87bd64222e831fd610270cd7ea250554"
        "9758bf75c05a994a6d034f65f8f0e6fd"
        "caeab1a34d4a6b4b636e070a38bce737"
    },
    {
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaa",
        "dddddddddddddddddddddddddddddddd"
        "dddddddddddddddddddddddddddddddd"
        "dddddddddddddddddddddddddddddddd"
        "dddd",
        "fa73b0089d56a284efb0f0756c890be9"
        "b1b5dbdd8ee81a3655f83e33b2279d39"
        "bf3e848279a722c806b485a47e67c807"
        "b946a337bee8942674278859e13292fb"
    },
    {
        "0102030405060708090a0b0c0d0e0f10"
        "111213141516171819",
        "cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd"
        "cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd"
        "cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd"
        "cdcd",
        "b0ba465637458c6990e5a8c5f61d4af7"
        "e576d97ff94b872de76f8050361ee3db"
        "a91ca5c11aa25eb4d679275cc5788063"
        "a5f19741120c4f2de2adebeb10a298dd"
    },
    {
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaa