
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef MASTERNODEMAN_H
#define MASTERNODEMAN_H

#include "bignum.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "core.h"
#include "util.h"
#include "script.h"
#include "base58.h"
#include "main.h"
#include "masternode.h"

#define MASTERNODES_DUMP_SECONDS               (15*60)

using namespace std;

class CMasternodeMan;

extern CMasternodeMan mnodeman;
extern std::vector<CTxIn> vecMasternodeAskedFor;
extern map<uint256, CMasternodePaymentWinner> mapSeenMasternodeVotes;
extern map<int64_t, uint256> mapCacheBlockHashes;

void DumpMasternodes();

/** Access to the MN database (masternodes.dat) */
class CMasternodeDB
{
private:
    boost::filesystem::path pathMN;
public:
    CMasternodeDB();
    bool Write(const CMasternodeMan &mnodemanToSave);
    bool Read(CMasternodeMan& mnodemanToLoad);
};

class CMasternodeMan
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    // map to hold all MNs
    std::vector<CMasternode> vMasternodes;

    // keep track of latest time whem vMasternodes was changed
    int64_t lastTimeChanged;

public:

    IMPLEMENT_SERIALIZE
    (
        // serialized format:
        // * version byte (currently 0)
        // * masternodes vector
        {
                LOCK(cs);
                unsigned char nVersion = 0;
                READWRITE(nVersion);
                READWRITE(lastTimeChanged);
                READWRITE(vMasternodes);
        }
    )

    CMasternodeMan();
    CMasternodeMan(CMasternodeMan& other);

    // Find an entry
    CMasternode* Find(const CTxIn& vin);

    // Find a random entry
    CMasternode* FindRandom();

    //Find an entry thta do not match every entry provided vector
    CMasternode* FindNotInVec(const std::vector<CTxIn> &vVins);

    // Add an entry
    bool Add(CMasternode &mn);

    // Check all masternodes
    void Check();

    // Check all masternodes and remove inactive
    void CheckAndRemove();

    // Clear masternode vector
    void Clear() { vMasternodes.clear(); lastTimeChanged = 0; }

    // Return the number of (unique) masternodes
    int size() { return vMasternodes.size(); }

    // Get the current winner for this block
    CMasternode* GetCurrentMasterNode(int mod=1, int64_t nBlockHeight=0, int minProtocol=0);

    int GetMasternodeRank(const CTxIn &vin, int64_t nBlockHeight, int minProtocol=0);

    int CountMasternodesAboveProtocol(int protocolVersion);

    int CountEnabled();

    std::vector<CMasternode> GetFullMasternodeVector() { Check(); return vMasternodes; }

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

    void UpdateLastTimeChanged() { lastTimeChanged = GetAdjustedTime(); }

    bool UpdateNeeded() { return lastTimeChanged < GetAdjustedTime() - MASTERNODE_REMOVAL_SECONDS; }

};

#endif
