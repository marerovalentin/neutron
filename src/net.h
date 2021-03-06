// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_H
#define BITCOIN_NET_H

#include "addrman.h"
#include "key.h"
#include "keystore.h"
#include "netaddress.h"
#include "protocol.h"
#include "main.h"
#include "mruset.h"
#include "random.h"
#include "scheduler.h"
#include "script.h"
#include "streams.h"
#include "threadinterrupt.h"
#include "timedata.h"
#include "utiltime.h"

#include <deque>
#include <thread>

#ifndef WIN32
#include <arpa/inet.h>
#endif

#include <boost/array.hpp>
#include <boost/foreach.hpp>
#include <openssl/rand.h>


class CTxIn;
class CTxOut;
class CRequestTracker;
class CScheduler;
class CNode;
class CBlockIndex;
extern int nBestHeight;

/** Run the feeler connection loop once every 2 minutes or 120 seconds. **/
static const int FEELER_INTERVAL = 120;
/** The maximum number of new addresses to accumulate before announcing. */
static const unsigned int MAX_ADDR_TO_SEND = 1000;
/** Maximum length of incoming protocol messages (no message over 2 MiB is currently acceptable). */
static const unsigned int MAX_PROTOCOL_MESSAGE_LENGTH = 2 * 1024 * 1024;
/** Maximum number of automatic outgoing nodes */
static const int MAX_OUTBOUND_CONNECTIONS = 64;
/** Maximum number of addnode outgoing nodes */
static const int MAX_ADDNODE_CONNECTIONS = 8;
/** -listen default */
static const bool DEFAULT_LISTEN = true;
/** -upnp default */
#ifdef USE_UPNP
static const bool DEFAULT_UPNP = USE_UPNP;
#else
static const bool DEFAULT_UPNP = false;
#endif
/** The maximum number of peer connections to maintain. */
static const unsigned int DEFAULT_MAX_PEER_CONNECTIONS = 125;

inline unsigned int ReceiveFloodSize() { return 1000*GetArg("-maxreceivebuffer", 5*1000); }
inline unsigned int SendBufferSize() { return 1000*GetArg("-maxsendbuffer", 1*1000); }

typedef std::map<CSubNet, int64_t> banmap_t;

bool RecvLine(SOCKET hSocket, std::string& strLine);
bool GetMyExternalIP(CNetAddr& ipRet);
void AddressCurrentlyConnected(const CService& addr);
CNode* FindNode(const CNetAddr& ip);
CNode* FindNode(const std::string addrName);
CNode* FindNode(const CService& ip);
void MapPort();
unsigned short GetListenPort();
bool BindListenPort(const CService &bindAddr, std::string& strError=REF(std::string()));
void SocketSendData(CNode *pnode);

typedef int NodeId;

#ifdef WIN32
// Win32 LevelDB doesn't use filedescriptors, and the ones used for
// accessing block files don't count towards the fd_set size limit
// anyway.
#define MIN_CORE_FILEDESCRIPTORS 0
#else
#define MIN_CORE_FILEDESCRIPTORS 150
#endif

enum
{
    LOCAL_NONE,   // unknown
    LOCAL_IF,     // address a local interface listens on
    LOCAL_BIND,   // address explicit bound to
    LOCAL_UPNP,   // address reported by UPnP
    LOCAL_HTTP,   // address reported by whatismyip.com and similar
    LOCAL_MANUAL, // address explicitly specified (-externalip=)

    LOCAL_MAX
};

void SetLimited(enum Network net, bool fLimited = true);
bool IsLimited(enum Network net);
bool IsLimited(const CNetAddr& addr);
bool AddLocal(const CService& addr, int nScore = LOCAL_NONE);
bool AddLocal(const CNetAddr& addr, int nScore = LOCAL_NONE);
bool SeenLocal(const CService& addr);
bool IsLocal(const CService& addr);
bool GetLocal(CService &addr, const CNetAddr *paddrPeer = NULL);
bool IsReachable(const CNetAddr &addr);
void SetReachable(enum Network net, bool fFlag = true);
CAddress GetLocalAddress(const CNetAddr *paddrPeer = NULL);


class CRequestTracker
{
public:
    void (*fn)(void*, CDataStream&);
    void* param1;

    explicit CRequestTracker(void (*fnIn)(void*, CDataStream&)=NULL, void* param1In=NULL)
    {
        fn = fnIn;
        param1 = param1In;
    }

    bool IsNull()
    {
        return fn == NULL;
    }
};


/** Thread types */
enum threadId
{
    THREAD_SOCKETHANDLER,
    THREAD_OPENCONNECTIONS,
    THREAD_MESSAGEHANDLER,
    THREAD_RPCLISTENER,
    THREAD_UPNP,
    THREAD_DNSSEED,
    THREAD_ADDEDCONNECTIONS,
    THREAD_RPCHANDLER,
    THREAD_STAKE_MINER,
    THREAD_MINER,
    THREAD_MAX
};

extern bool fClient;
extern bool fDiscover;
extern bool fListen;
extern bool fUseUPnP;
extern uint64_t nLocalServices;
extern uint64_t nLocalHostNonce;
extern CAddress addrSeenByPeer;
extern boost::array<int, THREAD_MAX> vnThreadsRunning;
extern CAddrMan addrman;

extern std::vector<CNode*> vNodes;
extern CCriticalSection cs_vNodes;
extern std::map<CInv, CDataStream> mapRelay;
extern std::deque<std::pair<int64_t, CInv> > vRelayExpiration;
extern CCriticalSection cs_mapRelay;
extern std::map<CInv, int64_t> mapAlreadyAskedFor;


extern NodeId nLastNodeId;
extern CCriticalSection cs_nLastNodeId;




class CConnman
{
public:

    enum NumConnections {
        CONNECTIONS_NONE = 0,
        CONNECTIONS_IN = (1U << 0),
        CONNECTIONS_OUT = (1U << 1),
        CONNECTIONS_ALL = (CONNECTIONS_IN | CONNECTIONS_OUT),
    };

    struct Options
    {
        // ServiceFlags nLocalServices = NODE_NONE;
        // ServiceFlags nRelevantServices = NODE_NONE;
        int nMaxConnections = 0;
        int nMaxOutbound = 0;
        int nMaxAddnode = 0;
        int nMaxFeeler = 0;
        int nBestHeight = 0;
        // CClientUIInterface* uiInterface = nullptr;
        unsigned int nSendBufferMaxSize = 0;
        unsigned int nReceiveFloodSize = 0;
        uint64_t nMaxOutboundTimeframe = 0;
        uint64_t nMaxOutboundLimit = 0;
    };
    CConnman(uint64_t seed0, uint64_t seed1);
    ~CConnman();
    bool Start(CScheduler& scheduler, Options connOptions);
    void Stop();
    void Interrupt();
    // bool BindListenPort(const CService &bindAddr, std::string& strError, bool fWhitelisted = false);
    // bool GetNetworkActive() const { return fNetworkActive; };
    // void SetNetworkActive(bool active);
    bool OpenNetworkConnection(const CAddress& addrConnect, CSemaphoreGrant *grantOutbound = NULL, const char *strDest = NULL, bool fOneShot = false);
    // bool OpenMasternodeConnection(const CAddress& addrConnect);
    // bool CheckIncomingNonce(uint64_t nonce);

    // struct CFullyConnectedOnly {
    //     bool operator() (const CNode* pnode) const {
    //         return NodeFullyConnected(pnode);
    //     }
    // };

    // constexpr static const CFullyConnectedOnly FullyConnectedOnly{};

    // struct CAllNodes {
    //     bool operator() (const CNode*) const {return true;}
    // };

    // constexpr static const CAllNodes AllNodes{};

    // bool ForNode(NodeId id, std::function<bool(const CNode* pnode)> cond, std::function<bool(CNode* pnode)> func);
    // bool ForNode(const CService& addr, std::function<bool(const CNode* pnode)> cond, std::function<bool(CNode* pnode)> func);

    // template<typename Callable>
    // bool ForNode(const CService& addr, Callable&& func)
    // {
    //     return ForNode(addr, FullyConnectedOnly, func);
    // }

    // template<typename Callable>
    // bool ForNode(NodeId id, Callable&& func)
    // {
    //     return ForNode(id, FullyConnectedOnly, func);
    // }

    // bool IsConnected(const CService& addr, std::function<bool(const CNode* pnode)> cond)
    // {
    //     return ForNode(addr, cond, [](CNode* pnode){
    //         return true;
    //     });
    // }

    // bool IsMasternodeOrDisconnectRequested(const CService& addr);

    // void PushMessage(CNode* pnode, CSerializedNetMsg&& msg);

    // template<typename Condition, typename Callable>
    // bool ForEachNodeContinueIf(const Condition& cond, Callable&& func)
    // {
    //     LOCK(cs_vNodes);
    //     for (auto&& node : vNodes)
    //         if (cond(node))
    //             if(!func(node))
    //                 return false;
    //     return true;
    // };

    // template<typename Callable>
    // bool ForEachNodeContinueIf(Callable&& func)
    // {
    //     return ForEachNodeContinueIf(FullyConnectedOnly, func);
    // }

    // template<typename Condition, typename Callable>
    // bool ForEachNodeContinueIf(const Condition& cond, Callable&& func) const
    // {
    //     LOCK(cs_vNodes);
    //     for (const auto& node : vNodes)
    //         if (cond(node))
    //             if(!func(node))
    //                 return false;
    //     return true;
    // };

    // template<typename Callable>
    // bool ForEachNodeContinueIf(Callable&& func) const
    // {
    //     return ForEachNodeContinueIf(FullyConnectedOnly, func);
    // }

    // template<typename Condition, typename Callable>
    // void ForEachNode(const Condition& cond, Callable&& func)
    // {
    //     LOCK(cs_vNodes);
    //     for (auto&& node : vNodes) {
    //         if (cond(node))
    //             func(node);
    //     }
    // };

    // template<typename Callable>
    // void ForEachNode(Callable&& func)
    // {
    //     ForEachNode(FullyConnectedOnly, func);
    // }

    // template<typename Condition, typename Callable>
    // void ForEachNode(const Condition& cond, Callable&& func) const
    // {
    //     LOCK(cs_vNodes);
    //     for (auto&& node : vNodes) {
    //         if (cond(node))
    //             func(node);
    //     }
    // };

    // template<typename Callable>
    // void ForEachNode(Callable&& func) const
    // {
    //     ForEachNode(FullyConnectedOnly, func);
    // }

    // template<typename Condition, typename Callable, typename CallableAfter>
    // void ForEachNodeThen(const Condition& cond, Callable&& pre, CallableAfter&& post)
    // {
    //     LOCK(cs_vNodes);
    //     for (auto&& node : vNodes) {
    //         if (cond(node))
    //             pre(node);
    //     }
    //     post();
    // };

    // template<typename Callable, typename CallableAfter>
    // void ForEachNodeThen(Callable&& pre, CallableAfter&& post)
    // {
    //     ForEachNodeThen(FullyConnectedOnly, pre, post);
    // }

    // template<typename Condition, typename Callable, typename CallableAfter>
    // void ForEachNodeThen(const Condition& cond, Callable&& pre, CallableAfter&& post) const
    // {
    //     LOCK(cs_vNodes);
    //     for (auto&& node : vNodes) {
    //         if (cond(node))
    //             pre(node);
    //     }
    //     post();
    // };

    // template<typename Callable, typename CallableAfter>
    // void ForEachNodeThen(Callable&& pre, CallableAfter&& post) const
    // {
    //     ForEachNodeThen(FullyConnectedOnly, pre, post);
    // }

    // std::vector<CNode*> CopyNodeVector(std::function<bool(const CNode* pnode)> cond);
    // std::vector<CNode*> CopyNodeVector();
    // void ReleaseNodeVector(const std::vector<CNode*>& vecNodes);

    // void RelayTransaction(const CTransaction& tx);
    // void RelayTransaction(const CTransaction& tx, const CDataStream& ss);
    // void RelayInv(CInv &inv, const int minProtoVersion = MIN_PEER_PROTO_VERSION);

    // // Addrman functions
    // size_t GetAddressCount() const;
    // void SetServices(const CService &addr, ServiceFlags nServices);
    // void MarkAddressGood(const CAddress& addr);
    // void AddNewAddress(const CAddress& addr, const CAddress& addrFrom, int64_t nTimePenalty = 0);
    // void AddNewAddresses(const std::vector<CAddress>& vAddr, const CAddress& addrFrom, int64_t nTimePenalty = 0);
    // std::vector<CAddress> GetAddresses();
    // void AddressCurrentlyConnected(const CService& addr);

    // // Denial-of-service detection/prevention
    // // The idea is to detect peers that are behaving
    // // badly and disconnect/ban them, but do it in a
    // // one-coding-mistake-won't-shatter-the-entire-network
    // // way.
    // // IMPORTANT:  There should be nothing I can give a
    // // node that it will forward on that will make that
    // // node's peers drop it. If there is, an attacker
    // // can isolate a node and/or try to split the network.
    // // Dropping a node for sending stuff that is invalid
    // // now but might be valid in a later version is also
    // // dangerous, because it can cause a network split
    // // between nodes running old code and nodes running
    // // new code.
    void Ban(const CNetAddr& addr, int64_t bantimeoffset);
    void Ban(const CSubNet& subNet, int64_t bantimeoffset);
    void ClearBanned(); // needed for unit testing
    bool IsBanned(CNetAddr ip);
    bool IsBanned(CSubNet subnet);
    bool Unban(const CNetAddr &ip);
    bool Unban(const CSubNet &ip);
    void GetBanned(banmap_t &banmap);
    void SetBanned(const banmap_t &banmap);

    void AddOneShot(const std::string& strDest);

    bool AddNode(const std::string& node);
    bool RemoveAddedNode(const std::string& node);
    // bool AddPendingMasternode(const CService& addr);
    // std::vector<AddedNodeInfo> GetAddedNodeInfo();

    // size_t GetNodeCount(NumConnections num);
    // void GetNodeStats(std::vector<CNodeStats>& vstats);
    // bool DisconnectNode(const std::string& node);
    // bool DisconnectNode(NodeId id);

    // unsigned int GetSendBufferSize() const;

    // void AddWhitelistedRange(const CSubNet &subnet);

    // ServiceFlags GetLocalServices() const;

    // //!set the max outbound target in bytes
    // void SetMaxOutboundTarget(uint64_t limit);
    // uint64_t GetMaxOutboundTarget();

    // //!set the timeframe for the max outbound target
    // void SetMaxOutboundTimeframe(uint64_t timeframe);
    // uint64_t GetMaxOutboundTimeframe();

    // //!check if the outbound target is reached
    // // if param historicalBlockServingLimit is set true, the function will
    // // response true if the limit for serving historical blocks has been reached
    // bool OutboundTargetReached(bool historicalBlockServingLimit);

    // //!response the bytes left in the current max outbound cycle
    // // in case of no limit, it will always response 0
    // uint64_t GetOutboundTargetBytesLeft();

    // //!response the time in second left in the current max outbound cycle
    // // in case of no limit, it will always response 0
    // uint64_t GetMaxOutboundTimeLeftInCycle();

    // uint64_t GetTotalBytesRecv();
    // uint64_t GetTotalBytesSent();

    // void SetBestHeight(int height);
    // int GetBestHeight() const;

    // /** Get a unique deterministic randomizer. */
    // CSipHasher GetDeterministicRandomizer(uint64_t id) const;

    // unsigned int GetReceiveFloodSize() const;

    // void WakeMessageHandler();

    CNode* ConnectNode(CAddress addrConnect, const char *strDest = NULL, bool darkSendMaster=false); // NTRN TODO - eventually make private

private:
    // struct ListenSocket {
    //     SOCKET socket;
    //     bool whitelisted;

    //     ListenSocket(SOCKET socket_, bool whitelisted_) : socket(socket_), whitelisted(whitelisted_) {}
    // };

    void ThreadOpenAddedConnections();
    void ThreadOpenAddedConnections2();
    void ProcessOneShot();
    void ThreadOpenConnections();
    void ThreadOpenConnections2();
    // void ThreadMessageHandler();
    // void AcceptConnection(const ListenSocket& hListenSocket);
    void ThreadSocketHandler();
    void ThreadSocketHandler2();
    void ThreadDNSAddressSeed();
    // void ThreadOpenMasternodeConnections();

    // uint64_t CalculateKeyedNetGroup(const CAddress& ad) const;

    // CNode* FindNode(const CNetAddr& ip);
    // CNode* FindNode(const CSubNet& subNet);
    // CNode* FindNode(const std::string& addrName);
    // CNode* FindNode(const CService& addr);

    // bool AttemptToEvictConnection();
    // CNode* ConnectNode(CAddress addrConnect, const char *strDest = NULL, bool darkSendMaster=false);
    // bool IsWhitelistedRange(const CNetAddr &addr);

    // void DeleteNode(CNode* pnode);

    // NodeId GetNewNodeId();

    // size_t SocketSendData(CNode *pnode) const;
    //!check is the banlist has unwritten changes
    bool BannedSetIsDirty();
    //!set the "dirty" flag for the banlist
    void SetBannedSetDirty(bool dirty=true);
    //!clean unused entries (if bantime has expired)
    void SweepBanned();
    void DumpAddresses();
    void DumpData();
    // void DumpBanlist();

    // // Network stats
    // void RecordBytesRecv(uint64_t bytes);
    // void RecordBytesSent(uint64_t bytes);

    // // Whether the node should be passed out in ForEach* callbacks
    // static bool NodeFullyConnected(const CNode* pnode);

    // // Network usage totals
    // CCriticalSection cs_totalBytesRecv;
    // CCriticalSection cs_totalBytesSent;
    // uint64_t nTotalBytesRecv;
    // uint64_t nTotalBytesSent;

    // // outbound limit & stats
    // uint64_t nMaxOutboundTotalBytesSentInCycle;
    // uint64_t nMaxOutboundCycleStartTime;
    // uint64_t nMaxOutboundLimit;
    // uint64_t nMaxOutboundTimeframe;

    // // Whitelisted ranges. Any node connecting from these is automatically
    // // whitelisted (as well as those connecting to whitelisted binds).
    // std::vector<CSubNet> vWhitelistedRange;
    // CCriticalSection cs_vWhitelistedRange;

    // unsigned int nSendBufferMaxSize;
    // unsigned int nReceiveFloodSize;

    // std::vector<ListenSocket> vhListenSocket;
    // std::atomic<bool> fNetworkActive;
    banmap_t setBanned;
    CCriticalSection cs_setBanned;
    bool setBannedIsDirty;
    bool fAddressesInitialized;
    // CAddrMan addrman;
    // std::deque<std::string> vOneShots;
    // CCriticalSection cs_vOneShots;
    // std::vector<std::string> vAddedNodes;
    // CCriticalSection cs_vAddedNodes;
    // std::vector<CService> vPendingMasternodes;
    // CCriticalSection cs_vPendingMasternodes;
    // std::vector<CNode*> vNodes;
    // std::list<CNode*> vNodesDisconnected;
    // mutable CCriticalSection cs_vNodes;
    // std::atomic<NodeId> nLastNodeId;

    // /** Services this instance offers */
    // ServiceFlags nLocalServices;

    // /** Services this instance cares about */
    // ServiceFlags nRelevantServices;

    CSemaphore *semOutbound;
    CSemaphore *semAddnode;
    // CSemaphore *semMasternodeOutbound;
    int nMaxConnections;
    int nMaxOutbound;
    int nMaxAddnode;
    int nMaxFeeler;
    // std::atomic<int> nBestHeight;
    // CClientUIInterface* clientInterface;

    /** SipHasher seeds for deterministic randomness */
    const uint64_t nSeed0, nSeed1;

    // /** flag for waking the message processor. */
    // bool fMsgProcWake;

    // std::condition_variable condMsgProc;
    std::mutex mutexMsgProc;
    std::atomic<bool> flagInterruptMsgProc;

    CThreadInterrupt interruptNet;

    std::thread threadDNSAddressSeed;
    std::thread threadSocketHandler;
    std::thread threadOpenAddedConnections;
    std::thread threadOpenConnections;
    // std::thread threadOpenMasternodeConnections;
    // std::thread threadMessageHandler;
};
extern std::unique_ptr<CConnman> g_connman;
// void Discover(boost::thread_group& threadGroup);
// void MapPort(bool fUseUPnP);
// unsigned short GetListenPort();
// bool BindListenPort(const CService &bindAddr, std::string& strError, bool fWhitelisted = false);




class CNodeStats
{
public:
    NodeId nodeid;
    uint64_t nServices;
    int64_t nLastSend;
    int64_t nLastRecv;
    int64_t nTimeConnected;
    std::string addrName;
    int nVersion;
    std::string cleanSubVer;
    std::string strSubVer;
    bool fInbound;
    int nStartingHeight;
    int nMisbehavior;
};




class CNetMessage {
public:
    bool in_data;                   // parsing header (false) or data (true)

    CDataStream hdrbuf;             // partially received header
    CMessageHeader hdr;             // complete header
    unsigned int nHdrPos;

    CDataStream vRecv;              // received message data
    unsigned int nDataPos;

    int64_t nTime;                  // time (in microseconds) of message receipt.

    CNetMessage(int nTypeIn, int nVersionIn) : hdrbuf(nTypeIn, nVersionIn), vRecv(nTypeIn, nVersionIn) {
        hdrbuf.resize(24);
        in_data = false;
        nHdrPos = 0;
        nDataPos = 0;
        nTime = 0;
    }

    bool complete() const
    {
        if (!in_data)
            return false;
        return (hdr.nMessageSize == nDataPos);
    }

    void SetVersion(int nVersionIn)
    {
        hdrbuf.SetVersion(nVersionIn);
        vRecv.SetVersion(nVersionIn);
    }

    int readHeader(const char *pch, unsigned int nBytes);
    int readData(const char *pch, unsigned int nBytes);
};



/** Information about a peer */
class CNode
{
    friend class CConnman;
public:
    // socket
    uint64_t nServices;
    SOCKET hSocket;
    CDataStream ssSend;
    size_t nSendSize; // total size of all vSendMsg entries
    size_t nSendOffset; // offset inside the first vSendMsg already sent
    std::deque<CSerializeData> vSendMsg;
    CCriticalSection cs_vSend;

    std::deque<CInv> vRecvGetData;
    std::deque<CNetMessage> vRecvMsg;
    CCriticalSection cs_vRecvMsg;
    int nRecvVersion;

    int64_t nLastSend;
    int64_t nLastRecv;
    int64_t nLastSendEmpty;
    int64_t nTimeConnected;
    CAddress addr;
    std::string addrName;
    CService addrLocal;
    int nVersion;
    std::string cleanSubVer;
    std::string strSubVer;
    bool fOneShot;
    bool fClient;
    bool fInbound;
    bool fNetworkNode;
    bool fSuccessfullyConnected;
    bool fDisconnect;
    bool fRelayTxes;
    bool fDarkSendMaster;
    // If 'true' this node will be disconnected on CMasternodeMan::ProcessMasternodeConnections()
    bool fMasternode; // NTRN TODO - finish implementing this
    CSemaphoreGrant grantOutbound;
    CSemaphoreGrant grantMasternodeOutbound; // NTRN TODO - finish implementing this
    int nRefCount;
    NodeId id;

public:
    int nMisbehavior;
    std::vector<std::string> vecRequestsFulfilled; //keep track of what client has asked for
    std::map<uint256, CRequestTracker> mapRequests;
    CCriticalSection cs_mapRequests;
    uint256 hashContinue;
    CBlockIndex* pindexLastGetBlocksBegin;
    uint256 hashLastGetBlocksEnd;
    int nStartingHeight;

    // flood relay
    std::vector<CAddress> vAddrToSend;
    mruset<CAddress> setAddrKnown;
    bool fGetAddr;
    std::set<uint256> setKnown;
    uint256 hashCheckpointKnown; // ppcoin: known sent sync-checkpoint

    // inventory based relay
    mruset<CInv> setInventoryKnown;
    std::vector<CInv> vInventoryToSend;
    CCriticalSection cs_inventory;
    std::multimap<int64_t, CInv> mapAskFor;

    CNode(SOCKET hSocketIn, CAddress addrIn, std::string addrNameIn = "", bool fInboundIn = false);

    ~CNode();

private:
    CNode(const CNode&);
    void operator=(const CNode&);

public:
    NodeId GetId() const {
      return id;
    }

    int GetRefCount()
    {
        assert(nRefCount >= 0);
        return nRefCount;
    }

    // requires LOCK(cs_vRecvMsg)
    unsigned int GetTotalRecvSize()
    {
        unsigned int total = 0;
        BOOST_FOREACH(const CNetMessage &msg, vRecvMsg)
            total += msg.vRecv.size() + 24;
        return total;
    }

    // requires LOCK(cs_vRecvMsg)
    bool ReceiveMsgBytes(const char *pch, unsigned int nBytes);

    // requires LOCK(cs_vRecvMsg)
    void SetRecvVersion(int nVersionIn)
    {
        nRecvVersion = nVersionIn;
        BOOST_FOREACH(CNetMessage &msg, vRecvMsg)
            msg.SetVersion(nVersionIn);
    }

    CNode* AddRef()
    {
        nRefCount++;
        return this;
    }

    void Release()
    {
        nRefCount--;
    }


    void AddAddressKnown(const CAddress& addr)
    {
        setAddrKnown.insert(addr);
    }

    void PushAddress(const CAddress& addr)
    {
        // Known checking here is only to save space from duplicates.
        // SendMessages will filter it again for knowns that were added
        // after addresses were pushed.
        if (addr.IsValid() && !setAddrKnown.count(addr)) {
            if (vAddrToSend.size() >= MAX_ADDR_TO_SEND) {
                vAddrToSend[insecure_rand() % vAddrToSend.size()] = addr;
            } else {
                vAddrToSend.push_back(addr);
            }
        }
    }


    void AddInventoryKnown(const CInv& inv)
    {
        {
            LOCK(cs_inventory);
            setInventoryKnown.insert(inv);
        }
    }

    void PushInventory(const CInv& inv)
    {
        {
            LOCK(cs_inventory);
            if (!setInventoryKnown.count(inv))
                vInventoryToSend.push_back(inv);
        }
    }

    void AskFor(const CInv& inv);

    // TODO: Document the postcondition of this function.  Is cs_vSend locked?
    void BeginMessage(const char* pszCommand) EXCLUSIVE_LOCK_FUNCTION(cs_vSend);

    // TODO: Document the postcondition of this function.  Is cs_vSend locked?
    void AbortMessage() UNLOCK_FUNCTION(cs_vSend);

    // TODO: Document the postcondition of this function.  Is cs_vSend locked?
    void EndMessage() UNLOCK_FUNCTION(cs_vSend);

    void PushVersion();


    void PushMessage(const char* pszCommand)
    {
        try
        {
            BeginMessage(pszCommand);
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1>
    void PushMessage(const char* pszCommand, const T1& a1)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1 << a2;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4 << a5;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4 << a5 << a6;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6, const T7& a7)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4 << a5 << a6 << a7;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6, const T7& a7, const T8& a8)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6, const T7& a7, const T8& a8, const T9& a9)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8 << a9;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }


    void PushRequest(const char* pszCommand,
                     void (*fn)(void*, CDataStream&), void* param1)
    {
        uint256 hashReply;
        RAND_bytes((unsigned char*)&hashReply, sizeof(hashReply));

        {
            LOCK(cs_mapRequests);
            mapRequests[hashReply] = CRequestTracker(fn, param1);
        }

        PushMessage(pszCommand, hashReply);
    }

    template<typename T1>
    void PushRequest(const char* pszCommand, const T1& a1,
                     void (*fn)(void*, CDataStream&), void* param1)
    {
        uint256 hashReply;
        RAND_bytes((unsigned char*)&hashReply, sizeof(hashReply));

        {
            LOCK(cs_mapRequests);
            mapRequests[hashReply] = CRequestTracker(fn, param1);
        }

        PushMessage(pszCommand, hashReply, a1);
    }

    template<typename T1, typename T2>
    void PushRequest(const char* pszCommand, const T1& a1, const T2& a2,
                     void (*fn)(void*, CDataStream&), void* param1)
    {
        uint256 hashReply;
        RAND_bytes((unsigned char*)&hashReply, sizeof(hashReply));

        {
            LOCK(cs_mapRequests);
            mapRequests[hashReply] = CRequestTracker(fn, param1);
        }

        PushMessage(pszCommand, hashReply, a1, a2);
    }

    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
    void PushMessage(const char* pszCommand, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6, const T7& a7, const T8& a8, const T9& a9, const T10& a10)
    {
        try
        {
            BeginMessage(pszCommand);
            ssSend << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8 << a9 << a10;
            EndMessage();
        }
        catch (...)
        {
            AbortMessage();
            throw;
        }
    }


    bool HasFulfilledRequest(std::string strRequest)
    {
        BOOST_FOREACH(std::string& type, vecRequestsFulfilled)
        {
            if(type == strRequest) return true;
        }
        return false;
    }

    void ClearFulfilledRequest(std::string strRequest)
    {
        std::vector<std::string>::iterator it = vecRequestsFulfilled.begin();
        while (it != vecRequestsFulfilled.end()) {
            if ((*it) == strRequest) {
                vecRequestsFulfilled.erase(it);
                return;
            }
            ++it;
        }
    }

    void FulfilledRequest(std::string strRequest)
    {
        if(HasFulfilledRequest(strRequest)) return;
        vecRequestsFulfilled.push_back(strRequest);
    }

    void PushGetBlocks(CBlockIndex* pindexBegin, uint256 hashEnd);
    bool IsSubscribed(unsigned int nChannel);
    void Subscribe(unsigned int nChannel, unsigned int nHops=0);
    void CancelSubscribe(unsigned int nChannel);
    void CloseSocketDisconnect();
    void Cleanup();


    // Denial-of-service detection/prevention
    // The idea is to detect peers that are behaving
    // badly and disconnect/ban them, but do it in a
    // one-coding-mistake-won't-shatter-the-entire-network
    // way.
    // IMPORTANT:  There should be nothing I can give a
    // node that it will forward on that will make that
    // node's peers drop it. If there is, an attacker
    // can isolate a node and/or try to split the network.
    // Dropping a node for sending stuff that is invalid
    // now but might be valid in a later version is also
    // dangerous, because it can cause a network split
    // between nodes running old code and nodes running
    // new code.
    bool Misbehaving(int howmuch); // 1 == a little, 100 == a lot

    void copyStats(CNodeStats &stats);
};

class CTransaction;
void RelayTransaction(const CTransaction& tx, const uint256& hash);
void RelayTransaction(const CTransaction& tx, const uint256& hash, const CDataStream& ss);
void RelayDarkSendFinalTransaction(const int sessionID, const CTransaction& txNew);
void RelayDarkSendIn(const std::vector<CTxIn>& in, const int64_t& nAmount, const CTransaction& txCollateral, const std::vector<CTxOut>& out);
void RelayDarkSendStatus(const int sessionID, const int newState, const int newEntriesCount, const int newAccepted, const std::string error="");
void RelayDarkSendElectionEntry(const CTxIn vin, const CService addr, const std::vector<unsigned char> vchSig, const int64_t nNow, const CPubKey pubkey, const CPubKey pubkey2, const int count, const int current, const int64_t lastUpdated, const int protocolVersion);
void SendDarkSendElectionEntry(const CTxIn vin, const CService addr, const std::vector<unsigned char> vchSig, const int64_t nNow, const CPubKey pubkey, const CPubKey pubkey2, const int count, const int current, const int64_t lastUpdated, const int protocolVersion);
void RelayDarkSendElectionEntryPing(const CTxIn vin, const std::vector<unsigned char> vchSig, const int64_t nNow, const bool stop);
void SendDarkSendElectionEntryPing(const CTxIn vin, const std::vector<unsigned char> vchSig, const int64_t nNow, const bool stop);
void RelayDarkSendCompletedTransaction(const int sessionID, const bool error, const std::string errorMessage);
void RelayDarkSendMasterNodeContestant();

/** Access to the (IP) address database (peers.dat) */
class CAddrDB
{
private:
    boost::filesystem::path pathAddr;
public:
    CAddrDB();
    bool Write(const CAddrMan& addr);
    bool Read(CAddrMan& addr);
};



/** Return a timestamp in the future (in microseconds) for exponentially distributed events. */
int64_t PoissonNextSend(int64_t nNow, int average_interval_seconds);

#endif // BITCOIN_NET_H
