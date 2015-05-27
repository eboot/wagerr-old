
// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef MASTERNODE_BUDGET_H
#define MASTERNODE_BUDGET_H

#include "main.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "masternode.h"
#include "masternode-pos.h"
//#include "timedata.h"

using namespace std;

class CBudgetManager;
class CFinalizedBudgetBroadcast;
class CFinalizedBudget;
class CFinalizedBudgetVote;
class CBudgetProposal;
class CBudgetProposalBroadcast;
class CBudgetVote;

#define VOTE_ABSTAIN  0
#define VOTE_YES      1
#define VOTE_NO       2

extern std::map<uint256, CBudgetProposalBroadcast> mapSeenMasternodeBudgetProposals;
extern std::map<uint256, CBudgetVote> mapSeenMasternodeBudgetVotes;
extern std::map<uint256, CFinalizedBudgetBroadcast> mapSeenFinalizedBudgets;
extern std::map<uint256, CFinalizedBudgetVote> mapSeenFinalizedBudgetVotes;

extern CBudgetManager budget;

void DumpBudgets();
void GetMasternodeBudgetEscrow(CScript& payee);

//Amount of blocks in a months period of time (using 2.6 minutes per)
int GetBudgetPaymentCycleBlocks();

/** Save Budget Manager (budget.dat)
 */
class CBudgetDB
{
private:
    boost::filesystem::path pathDB;
    std::string strMagicMessage;
public:
    enum ReadResult {
        Ok,
        FileError,
        HashReadError,
        IncorrectHash,
        IncorrectMagicMessage,
        IncorrectMagicNumber,
        IncorrectFormat
    };

    CBudgetDB();
    bool Write(const CBudgetManager &budgetToSave);
    ReadResult Read(CBudgetManager& budgetToLoad);
};


//
// Budget Manager : Contains all proposals for the budget
//
class CBudgetManager
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

public:
    // keep track of the scanning errors I've seen
    map<uint256, CBudgetProposal> mapProposals;
    map<uint256, CFinalizedBudget> mapFinalizedBudgets;
    
    CBudgetManager() {
        mapProposals.clear();
        mapFinalizedBudgets.clear();
    }

    void Sync(CNode* node);

    void Calculate();
    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
    void NewBlock();
    CBudgetProposal *Find(const std::string &strProposalName);
    CFinalizedBudget *Find(uint256 nHash);
    std::pair<std::string, std::string> GetVotes(std::string strProposalName);

    
    void CleanUp();

    int64_t GetTotalBudget();
    std::vector<CBudgetProposal*> GetBudget();
    std::vector<CFinalizedBudget*> GetFinalizedBudgets();
    bool IsBudgetPaymentBlock(int nBlockHeight);
    void AddProposal(CBudgetProposal& prop);
    void UpdateProposal(CBudgetVote& vote);
    void AddFinalizedBudget(CFinalizedBudget& prop);
    void UpdateFinalizedBudget(CFinalizedBudgetVote& vote);
    bool PropExists(uint256 nHash);
    bool IsTransactionValid(const CTransaction& txNew, int nBlockHeight);
    std::string GetRequiredPaymentsString(int64_t nBlockHeight);

    void Clear(){
        printf("Not implemented\n");
    }
    void CheckAndRemove(){
        printf("Not implemented\n");
        //replace cleanup
    }
    std::string ToString() {return "not implemented";}


    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(mapSeenMasternodeBudgetProposals);
        READWRITE(mapSeenMasternodeBudgetVotes);
        READWRITE(mapSeenFinalizedBudgets);
        READWRITE(mapSeenFinalizedBudgetVotes);

        READWRITE(mapProposals);
        READWRITE(mapFinalizedBudgets);
    }
};

//
// Finalized Budget : Contains the suggested proposals to pay on a given block
//

class CFinalizedBudget
{

private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

public:
    CTxIn vin;
    std::string strBudgetName;
    int nBlockStart;
    std::vector<uint256> vecProposals;
    map<uint256, CFinalizedBudgetVote> mapVotes;

    CFinalizedBudget();  
    CFinalizedBudget(const CFinalizedBudget& other);

    void Sync(CNode* node);

    void Clean(CFinalizedBudgetVote& vote);
    void AddOrUpdateVote(CFinalizedBudgetVote& vote);
    double GetScore();
    bool HasMinimumRequiredSupport();

    bool IsValid();

    std::string GetName() {return strBudgetName; }
    std::string GetProposals();
    int GetBlockStart() {return nBlockStart;}
    int GetBlockEnd() {return nBlockStart + (int)(vecProposals.size()-1);}
    std::string GetSubmittedBy() {return vin.prevout.ToStringShort();}
    int GetVoteCount() {return (int)mapVotes.size();}
    bool IsTransactionValid(const CTransaction& txNew, int nBlockHeight);
    uint256 GetProposalByBlock(int64_t nBlockHeight)
    {
        int i = nBlockHeight - GetBlockStart();
        if(i < 0) return 0;
        if(i > (int)vecProposals.size()-1) return 0;
        return vecProposals[i];
    }

    uint256 GetHash(){
        /* 
            vin is not included on purpose
                - Any masternode can make a proposal and the hashes should match regardless of who made it. 
                - Someone could hyjack a new proposal by changing the vin and the signature check will fail. 
                  However, the network will still propagate the correct version and the incorrect one will be rejected. 
        */
                
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << strBudgetName;
        ss << nBlockStart;
        ss << vecProposals;

        uint256 h1 = ss.GetHash();        
        return h1;
    }

    ADD_SERIALIZE_METHODS;

    //for saving to the serialized db
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(LIMITED_STRING(strBudgetName, 20));
        READWRITE(vin);
        READWRITE(nBlockStart);
        READWRITE(vecProposals);

        READWRITE(mapVotes);
    }
};

// FinalizedBudget are cast then sent to peers with this object, which leaves the votes out
class CFinalizedBudgetBroadcast : public CFinalizedBudget
{
private:
    std::vector<unsigned char> vchSig;
    
public:
    CFinalizedBudgetBroadcast();
    CFinalizedBudgetBroadcast(const CFinalizedBudget& other);
    CFinalizedBudgetBroadcast(CTxIn& vinIn, std::string strBudgetNameIn, int nBlockStartIn, std::vector<uint256> vecProposalsIn);

    bool Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode);
    bool SignatureValid();
    void Relay();

    ADD_SERIALIZE_METHODS;

    //for propagating messages
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        //for syncing with other clients
        READWRITE(LIMITED_STRING(strBudgetName, 20));
        READWRITE(vin);
        READWRITE(nBlockStart);
        READWRITE(vecProposals);
        READWRITE(vchSig);
    }  
};

//
// CFinalizedBudgetVote - Allow a masternode node to vote and broadcast throughout the network
//

class CFinalizedBudgetVote
{
public:
    CTxIn vin;
    uint256 nBudgetHash;
    int64_t nTime;
    std::vector<unsigned char> vchSig;

    CFinalizedBudgetVote();
    CFinalizedBudgetVote(CTxIn vinIn, uint256 nBudgetHashIn);

    bool Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode);
    bool SignatureValid();
    void Relay();

    uint256 GetHash(){
        return Hash(BEGIN(vin), END(vin), BEGIN(nBudgetHash), END(nBudgetHash), BEGIN(nTime), END(nTime));
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vin);
        READWRITE(nBudgetHash);
        READWRITE(nTime);
        READWRITE(vchSig);
    }

    

};

//
// Budget Proposal : Contains the masternode votes for each budget
//

class CBudgetProposal
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
    int64_t nAlloted;

public:
    std::string strProposalName;
    
    /* 
        json object with name, short-description, long-description, pdf-url and any other info
        This allows the proposal website to stay 100% decentralized
    */
    std::string strURL; 
    CTxIn vin;
    int nBlockStart;
    int nBlockEnd;
    int64_t nAmount;
    CScript address;

    map<uint256, CBudgetVote> mapVotes;
    //cache object

    CBudgetProposal();  
    CBudgetProposal(const CBudgetProposal& other);
    CBudgetProposal(CTxIn vinIn, std::string strProposalNameIn, std::string strURLIn, int nBlockStartIn, int nBlockEndIn, CScript addressIn, CAmount nAmountIn);

    void Sync(CNode* node);

    void Calculate();
    void AddOrUpdateVote(CBudgetVote& vote);
    bool HasMinimumRequiredSupport();
    std::pair<std::string, std::string> GetVotes();

    bool IsValid();

    std::string GetName() {return strProposalName; }
    std::string GetURL() {return strURL; }
    int GetBlockStart() {return nBlockStart;}
    int GetBlockEnd() {return nBlockEnd;}
    CScript GetPayee() {return address;}
    int GetTotalPaymentCount();
    int GetRemainingPaymentCount();
    int GetBlockStartCycle();
    int GetBlockCurrentCycle();
    int GetBlockEndCycle();
    double GetRatio();
    int GetYeas();
    int GetNays();
    int GetAbstains();
    int64_t GetAmount() {return nAmount;}

    void SetAllotted(int64_t nAllotedIn) {nAlloted = nAllotedIn;}
    int64_t GetAllotted() {return nAlloted;}

    uint256 GetHash(){
        /* 
            vin is not included on purpose
                - Any masternode can make a proposal and the hashes should match regardless of who made it. 
                - Someone could hyjack a new proposal by changing the vin and the signature check will fail. 
                  However, the network will still propagate the correct version and the incorrect one will be rejected. 
        */
                
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << strProposalName;
        ss << strURL;
        ss << nBlockStart;
        ss << nBlockEnd;
        ss << nAmount;
        ss << address;
        uint256 h1 = ss.GetHash();
        
        return h1;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        //for syncing with other clients
        READWRITE(LIMITED_STRING(strProposalName, 20));
        READWRITE(LIMITED_STRING(strURL, 64));
        READWRITE(vin);
        READWRITE(nBlockStart);
        READWRITE(nBlockEnd);
        READWRITE(nAmount);
        READWRITE(address);

        //for saving to the serialized db
        READWRITE(mapVotes);
    }
};

// Proposals are cast then sent to peers with this object, which leaves the votes out
class CBudgetProposalBroadcast : public CBudgetProposal
{
private:
    std::vector<unsigned char> vchSig;
    
public:
    CBudgetProposalBroadcast();
    CBudgetProposalBroadcast(const CBudgetProposal& other);
    CBudgetProposalBroadcast(CTxIn vinIn, std::string strProposalNameIn, std::string strURL, int nPaymentCount, CScript addressIn, CAmount nAmountIn, int nBlockStartIn);

    bool Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode);
    bool SignatureValid();
    void Relay();

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        //for syncing with other clients

        READWRITE(LIMITED_STRING(strProposalName, 20));
        READWRITE(LIMITED_STRING(strURL, 64));
        READWRITE(vin);
        READWRITE(nBlockStart);
        READWRITE(nBlockEnd);
        READWRITE(nAmount);
        READWRITE(address);
        READWRITE(vchSig);
    }  
};

//
// CBudgetVote - Allow a masternode node to vote and broadcast throughout the network
//

class CBudgetVote
{
public:
    CTxIn vin;
    uint256 nProposalHash;
    int nVote;
    int64_t nTime;
    std::vector<unsigned char> vchSig;

    CBudgetVote();
    CBudgetVote(CTxIn vin, uint256 nProposalHash, int nVoteIn);

    bool Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode);
    bool SignatureValid();
    void Relay();

    std::string GetVoteString() {
        std::string ret = "ABSTAIN";
        if(nVote == VOTE_YES) ret = "YES";
        if(nVote == VOTE_NO) ret = "NO";
        return ret;
    }

    uint256 GetHash(){
        return Hash(BEGIN(vin), END(vin), BEGIN(nProposalHash), END(nProposalHash), BEGIN(nVote), END(nVote), BEGIN(nTime), END(nTime));
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vin);
        READWRITE(nProposalHash);
        READWRITE(nVote);
        READWRITE(nTime);
        READWRITE(vchSig);
    }

    

};

#endif