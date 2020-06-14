// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "init.h"

#include "addrman.h"
#include "main.h"
#include "chainparams.h"
#include "txdb.h"
#include "rpcserver.h"
#include "net.h"
#include "key.h"
#include "pubkey.h"
#include "util.h"
#include "ui_interface.h"
#include "checkpoints.h"
#include "smessage.h"

#ifdef ENABLE_WALLET
#include "db.h"
#include "wallet.h"
#include "walletdb.h"
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/thread.hpp>
#include <openssl/crypto.h>

#ifndef WIN32
#include <signal.h>
#endif


using namespace std;
using namespace boost;

#ifdef ENABLE_WALLET
CWallet* pwalletMain = NULL;
int nWalletBackups = 10;
int nNewHeight;
#endif
CClientUIInterface uiInterface;
bool fConfChange;
unsigned int nNodeLifespan;
unsigned int nDerivationMethodIndex;
unsigned int nMinerSleep;
bool fUseFastIndex;
bool fOnlyTor = false;


//////////////////////////////////////////////////////////////////////////////
//
// Shutdown
//

//
// Thread management and startup/shutdown:
//
// The network-processing threads are all part of a thread group
// created by AppInit() or the Qt main() function.
//
// A clean exit happens when StartShutdown() or the SIGTERM
// signal handler sets fRequestShutdown, which triggers
// the DetectShutdownThread(), which interrupts the main thread group.
// DetectShutdownThread() then exits, which causes AppInit() to
// continue (it .joins the shutdown thread).
// Shutdown() is then
// called to clean up database connections, and stop other
// threads that should only be stopped after the main network-processing
// threads have exited.
//
// Note that if running -daemon the parent process returns from AppInit2
// before adding any threads to the threadGroup, so .join_all() returns
// immediately and the parent exits from main().
//
// Shutdown for Qt is very similar, only it uses a QTimer to detect
// fRequestShutdown getting set, and then does the normal Qt
// shutdown thing.
//

volatile bool fRequestShutdown = false;

void StartShutdown()
{
    fRequestShutdown = true;
}
bool ShutdownRequested()
{
    return fRequestShutdown;
}

static boost::scoped_ptr<ECCVerifyHandle> globalVerifyHandle;

void Shutdown()
{
	fRequestShutdown = true; // Needed when we shutdown the wallet
    LogPrintf("Shutdown : In progress...\n");
    static CCriticalSection cs_Shutdown;
    TRY_LOCK(cs_Shutdown, lockShutdown);
    if (!lockShutdown) return;

    RenameThread("WayaWolfCoin-shutoff");
    mempool.AddTransactionsUpdated(1);
    StopRPCThreads();
    SecureMsgShutdown();

#ifdef ENABLE_WALLET
    ShutdownRPCMining();
    if (pwalletMain)
        bitdb.Flush(false);
#endif
    StopNode();
    UnregisterNodeSignals(GetNodeSignals());
    {
        LOCK(cs_main);
#ifdef ENABLE_WALLET
        if (pwalletMain)
            pwalletMain->SetBestChain(CBlockLocator(pindexBest));
#endif
    }
#ifdef ENABLE_WALLET
    if (pwalletMain)
        bitdb.Flush(true);
#endif
    boost::filesystem::remove(GetPidFile());
    UnregisterAllWallets();
#ifdef ENABLE_WALLET
    delete pwalletMain;
    pwalletMain = NULL;
#endif
    globalVerifyHandle.reset();
    ECC_Stop();
    LogPrintf("Shutdown : done\n");
}

//
// Signal handlers are very limited in what they are allowed to do, so:
//
void HandleSIGTERM(int)
{
    fRequestShutdown = true;
}

void HandleSIGHUP(int)
{
    fReopenDebugLog = true;
}

bool static InitError(const std::string &str)
{
    uiInterface.ThreadSafeMessageBox(str, "", CClientUIInterface::MSG_ERROR);
    return false;
}

bool static InitWarning(const std::string &str)
{
    uiInterface.ThreadSafeMessageBox(str, "", CClientUIInterface::MSG_WARNING);
    return true;
}

bool static Bind(const CService &addr, bool fError = true) {
    if (IsLimited(addr))
        return false;
    std::string strError;
    if (!BindListenPort(addr, strError)) {
        if (fError)
            return InitError(strError);
        return false;
    }
    return true;
}

// Core-specific options shared between UI and daemon
std::string HelpMessage()
{
    string strUsage = _("Options:") + "\n";
    strUsage += "  -?                     " + _("This help message") + "\n";
    strUsage += "  -conf=<file>           " + _("Specify configuration file (default: WayaWolfCoin.conf)") + "\n";
    strUsage += "  -pid=<file>            " + _("Specify pid file (default: WayaWolfCoind.pid)") + "\n";
    strUsage += "  -datadir=<dir>         " + _("Specify data directory") + "\n";
    strUsage += "  -wallet=<dir>          " + _("Specify wallet file (within data directory)") + "\n";
    strUsage += "  -dbcache=<n>           " + _("Set database cache size in megabytes (default: 100)") + "\n";
    strUsage += "  -dblogsize=<n>         " + _("Set database disk log size in megabytes (default: 100)") + "\n";
    strUsage += "  -timeout=<n>           " + _("Specify connection timeout in milliseconds (default: 5000)") + "\n";
    strUsage += "  -proxy=<ip:port>       " + _("Connect through SOCKS5 proxy") + "\n";
    strUsage += "  -tor=<ip:port>         " + _("Use proxy to reach tor hidden services (default: same as -proxy)") + "\n";
    strUsage += "  -dns                   " + _("Allow DNS lookups for -addnode, -seednode and -connect") + "\n";
    strUsage += "  -port=<port>           " + _("Listen for connections on <port> (default: 20995)") + "\n";
    strUsage += "  -maxconnections=<n>    " + _("Maintain at most <n> connections to peers (default: 125)") + "\n";
    strUsage += "  -addnode=<ip>          " + _("Add a node to connect to and attempt to keep the connection open") + "\n";
    strUsage += "  -connect=<ip>          " + _("Connect only to the specified node(s)") + "\n";
    strUsage += "  -seednode=<ip>         " + _("Connect to a node to retrieve peer addresses, and disconnect") + "\n";
    strUsage += "  -externalip=<ip>       " + _("Specify your own public address") + "\n";
    strUsage += "  -onlynet=<net>         " + _("Only connect to nodes in network <net> (IPv4, IPv6 or Tor)") + "\n";
    strUsage += "  -discover              " + _("Discover own IP address (default: 1 when listening and no -externalip)") + "\n";
    strUsage += "  -listen                " + _("Accept connections from outside (default: 1 if no -proxy or -connect)") + "\n";
    strUsage += "  -bind=<addr>           " + _("Bind to given address. Use [host]:port notation for IPv6") + "\n";
    strUsage += "  -dnsseed               " + _("Query for peer addresses via DNS lookup, if low on addresses (default: 1 unless -connect)") + "\n";
    strUsage += "  -forcednsseed          " + _("Always query for peer addresses via DNS lookup (default: 0)") + "\n";
    strUsage += "  -synctime              " + _("Sync time with other nodes. Disable if time on your system is precise e.g. syncing with NTP (default: 1)") + "\n";
    strUsage += "  -banscore=<n>          " + _("Threshold for disconnecting misbehaving peers (default: 100)") + "\n";
    strUsage += "  -bantime=<n>           " + _("Number of seconds to keep misbehaving peers from reconnecting (default: 86400)") + "\n";
    strUsage += "  -maxreceivebuffer=<n>  " + _("Maximum per-connection receive buffer, <n>*1000 bytes (default: 5000)") + "\n";
    strUsage += "  -maxsendbuffer=<n>     " + _("Maximum per-connection send buffer, <n>*1000 bytes (default: 1000)") + "\n";
#ifdef USE_UPNP
#if USE_UPNP
    strUsage += "  -upnp                  " + _("Use UP