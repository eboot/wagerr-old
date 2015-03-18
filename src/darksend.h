// Copyright (c) 2014-2015 The Darkcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DARKSEND_H
#define DARKSEND_H

#include "core.h"
#include "main.h"
#include "activemasternode.h"
#include "masternodeman.h"
#include "darksend-relay.h"

class CTxIn;
class CDarksendPool;
class CDarkSendSigner;
class CMasterNodeVote;
class CBitcoinAddress;
class CDarksendQueue;
class CDarksendBroadcastTx;
class CActiveMasternode;

// pool states for mixing
#define POOL_MAX_TRANSACTIONS                  3 // wait for X transactions to merge and publish
#define POOL_MAX_TRANSACTIONS_TESTNET          2 // wait for X transactions to merge and publish
#define POOL_STATUS_UNKNOWN                    0 // waiting for update
#define POOL_STATUS_IDLE                       1 // waiting for update
#define POOL_STATUS_QUEUE                      2 // waiting in a queue
#define POOL_STATUS_ACCEPTING_ENTRIES          3 // accepting entries
#define POOL_STATUS_FINALIZE_TRANSACTION       4 // master node will broadcast what it accepted
#define POOL_STATUS_SIGNING                    5 // check inputs/outputs, sign final tx
#define POOL_STATUS_TRANSMISSION               6 // transmit transaction
#define POOL_STATUS_ERROR                      7 // error
#define POOL_STATUS_SUCCESS                    8 // success

// status update message constants
#define MASTERNODE_ACCEPTED                    1
#define MASTERNODE_REJECTED                    0
#define MASTERNODE_RESET                       -1

#define DARKSEND_QUEUE_TIMEOUT                 120 // in seconds
#define DARKSEND_SIGNING_TIMEOUT               30 // in seconds
#define DARKSEND_DOWNGRADE_TIMEOUT             30 // in seconds

// used for anonymous relaying of inputs/outputs/sigs
#define DARKSEND_RELAY_IN                 1
#define DARKSEND_RELAY_OUT                2
#define DARKSEND_RELAY_SIG                3

extern CDarksendPool darkSendPool;
extern CDarkSendSigner darkSendSigner;
extern std::vector<CDarksendQueue> vecDarksendQueue;
extern std::string strMasterNodePrivKey;
extern map<uint256, CDarksendBroadcastTx> mapDarksendBroadcastTxes;
extern CActiveMasternode activeMasternode;

// get the Darksend chain depth for a given input
int GetInputDarksendRounds(CTxIn in, int rounds=0);

/** Holds an Darksend input
 */
class CTxDSIn : public CTxIn
{
public:
    bool fHasSig; // flag to indicate if signed
    int nSentTimes; //times we've sent this anonymously 

    CTxDSIn(const CTxIn& in)
    {
        prevout = in.prevout;
        scriptSig = in.scriptSig;
        prevPubKey = in.prevPubKey;
        nSequence = in.nSequence;
        nSentTimes = 0;
    }
};

/** Holds an Darksend output
 */
class CTxDSOut : public CTxOut
{
public:
    int nSentTimes; //times we've sent this anonymously 

    CTxDSOut(const CTxOut& out)
    {
        nValue = out.nValue;
        nRounds = out.nRounds;
        scriptPubKey = out.scriptPubKey;
        nSentTimes = 0;
    }
};

/** A clients transaction in the Darksend pool
 *  -- holds the input/output mapping for each user in the pool
 */
class CDarkSendEntry
{
public:
    bool isSet;
    std::vector<CTxDSIn> sev;
    std::vector<CTxDSOut> vout;
    int64_t amount;
    CTransaction collateral;
    CTransaction txSupporting;
    int64_t addedTime; // time in UTC milliseconds

    CDarkSendEntry()
    {
        isSet = false;
        collateral = CTransaction();
        amount = 0;
    }

    /// Add entries to use for Darksend
    bool Add(const std::vector<CTxIn> vinIn, int64_t amountIn, const CTransaction collateralIn, const std::vector<CTxOut> voutIn)
    {
        if(isSet){return false;}

        BOOST_FOREACH(const CTxIn& in, vinIn)
            sev.push_back(in);

        BOOST_FOREACH(const CTxOut& out, voutIn)
            vout.push_back(out);

        amount = amountIn;
        collateral = collateralIn;
        isSet = true;
        addedTime = GetTime();

        return true;
    }

    /// Is this Darksend expired?
    bool IsExpired()
    {
        return (GetTime() - addedTime) > DARKSEND_QUEUE_TIMEOUT;// 120 seconds
    }
};


/**
 * A currently inprogress Darksend merge and denomination information
 */
class CDarksendQueue
{
public:
    CTxIn vin;
    int64_t time;
    int nDenom;
    bool ready; //ready for submit
    std::vector<unsigned char> vchSig;

    //information used for the anonymous relay system
    int nBlockHeight;
    std::vector<unsigned char> vchRelaySig;
    std::string strSharedKey; // shared key

    CDarksendQueue()
    {
        nDenom = 0;
        vin = CTxIn();
        time = 0;
        vchSig.clear();
        vchRelaySig.clear();
        nBlockHeight = 0;
        strSharedKey = "";
        ready = false;
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(nDenom);
        READWRITE(vin);
        READWRITE(time);
        READWRITE(ready);
        READWRITE(vchSig);

        if(ready){
            READWRITE(vchRelaySig);
            READWRITE(nBlockHeight);
            READWRITE(strSharedKey);
        }
    )

    bool GetAddress(CService &addr)
    {
        CMasternode* pmn = mnodeman.Find(vin);
        if(pmn != NULL)
        {
            addr = pmn->addr;
            return true;
        }
        return false;
    }

    /// Get the protocol version
    bool GetProtocolVersion(int &protocolVersion)
    {
        CMasternode* pmn = mnodeman.Find(vin);
        if(pmn != NULL)
        {
            protocolVersion = pmn->protocolVersion;
            return true;
        }
        return false;
    }

    /// Set the 'strSharedKey' 
    void SetSharedKey(std::string strSharedKey);

    /** Sign this Darksend transaction
     *  \return true if all conditions are met:
     *     1) we have an active Masternode,
     *     2) we have a valid Masternode private key,
     *     3) we signed the message successfully, and
     *     4) we verified the message successfully
     */
    bool Sign();

    bool Relay();

    /// Is this Darksend expired?
    bool IsExpired()
    {
        return (GetTime() - time) > DARKSEND_QUEUE_TIMEOUT;// 120 seconds
    }

    /// Check if we have a valid Masternode address
    bool CheckSignature();

};

/** Helper class to store Darksend transaction (tx) information.
 */
class CDarksendBroadcastTx
{
public:
    CTransaction tx;
    CTxIn vin;
    vector<unsigned char> vchSig;
    int64_t sigTime;
};

/** Helper object for signing and checking signatures
 */
class CDarkSendSigner
{
public:
    /// Is the inputs associated with this public key? (and there is 1000 DRK - checking if valid masternode)
    bool IsVinAssociatedWithPubkey(CTxIn& vin, CPubKey& pubkey);
    /// Set the private/public key values, returns true if successful
    bool SetKey(std::string strSecret, std::string& errorMessage, CKey& key, CPubKey& pubkey);
    /// Sign the message, returns true if successful
    bool SignMessage(std::string strMessage, std::string& errorMessage, std::vector<unsigned char>& vchSig, CKey key);
    /// Verify the message, returns true if succcessful
    bool VerifyMessage(CPubKey pubkey, std::vector<unsigned char>& vchSig, std::string strMessage, std::string& errorMessage);
};

/** Build a transaction anonymously
 */
class CDSAnonTx
{
public:
    std::vector<CTxDSIn> vin; // collection of inputs
    std::vector<CTxOut> vout; // collection of outputs

    /// Is the transaction valid? (TODO: not defined - remove? or code?)
    bool IsTransactionValid();
    /// Add an output
    bool AddOutput(const CTxOut out);
    /// Add an input 
    bool AddInput(const CTxIn in);
    /// Clear Signatures
    bool ClearSigs();
    /// Add Signature
    bool AddSig(const CTxIn in);
    /// Count the number of entries in the transaction
    int CountEntries() {return (int)vin.size() + (int)vout.size();}
};

/// TODO: not defined - remove?
void ConnectToDarkSendMasterNodeWinner();


/** Used to keep track of current status of Darksend pool
 */
class CDarksendPool
{
public:

    std::vector<CDarkSendEntry> myEntries; // clients entries
    std::vector<CDarkSendEntry> entries; // Masternode entries
    CTransaction finalTransaction; // the finalized transaction ready for signing
    CDSAnonTx anonTx; // anonymous inputs/outputs
    bool fSubmitAnonymousFailed; // initally false, will change to true if when attempts > 5
    int nCountAttempts; // number of submitted attempts

    int64_t lastTimeChanged; // last time the 'state' changed, in UTC milliseconds
    int64_t lastAutoDenomination; // TODO; not used - Delete?

    unsigned int state; // should be one of the POOL_STATUS_XXX values
    unsigned int entriesCount;
    unsigned int lastEntryAccepted;
    unsigned int countEntriesAccepted;

    // where collateral should be made out to
    CScript collateralPubKey;

    std::vector<CTxIn> lockedCoins;

    uint256 masterNodeBlockHash;

    std::string lastMessage;
    bool completedTransaction;
    bool unitTest;
    CMasternode* pSubmittedToMasternode;

    int sessionID;
    int sessionDenom; //Users must submit an denom matching this
    int sessionUsers; //N Users have said they'll join
    bool sessionFoundMasternode; //If we've found a compatible Masternode
    int64_t sessionTotalValue; //used for autoDenom
    std::vector<CTransaction> vecSessionCollateral;

    int cachedLastSuccess;
    int cachedNumBlocks; //used for the overview screen
    int minBlockSpacing; //required blocks between mixes
    CTransaction txCollateral;

    int64_t lastNewBlock;

    //debugging data
    std::string strAutoDenomResult;

    // used for securing the anonymous relay system
    vector<unsigned char> vchMasternodeRelaySig;
    int nMasternodeBlockHeight;
    std::string strMasternodeSharedKey;
    int nTrickleInputsOutputs;

    CDarksendPool()
    {
        /* Darksend uses collateral addresses to trust parties entering the pool
            to behave themselves. If they don't it takes their money. */

        cachedLastSuccess = 0;
        cachedNumBlocks = 0;
        unitTest = false;
        txCollateral = CTransaction();
        minBlockSpacing = 1;
        lastNewBlock = 0;
        strMasternodeSharedKey = "";
        nTrickleInputsOutputs = 0;

        SetNull();
    }

    /** Process a Darksend message using the Darksend protocol
     * \param pfrom
     * \param strCommand lower case command string; valid values are:
     *        Command  | Description
     *        -------- | -----------------
     *        dsa      | Darksend Acceptable
     *        dsc      | Darksend Complete
     *        dsf      | Darksend Final tx
     *        dsi      | Darksend vIn
     *        dsq      | Darksend Queue
     *        dss      | Darksend Signal Final Tx
     *        dssu     | Darksend status update
     *        dssub    | Darksend Subscribe To
     * \param vRecv
     */
    void ProcessMessageDarksend(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

    void InitCollateralAddress(){
        std::string strAddress = "";
        if(Params().NetworkID() == CChainParams::MAIN) {
            strAddress = "Xq19GqFvajRrEdDHYRKGYjTsQfpV5jyipF";
        } else {
            strAddress = "y1EZuxhhNMAUofTBEeLqGE1bJrpC2TWRNp";
        }
        SetCollateralAddress(strAddress);
    }

    void SetMinBlockSpacing(int minBlockSpacingIn){
        minBlockSpacing = minBlockSpacingIn;
    }

    bool SetCollateralAddress(std::string strAddress);
    void Reset();
    bool Downgrade();
    bool TrickleInputsOutputs();

    void SetNull(bool clearEverything=false);

    void UnlockCoins();

    bool IsNull() const
    {
        return (state == POOL_STATUS_ACCEPTING_ENTRIES && entries.empty() && myEntries.empty());
    }

    int GetState() const
    {
        return state;
    }

    int GetEntriesCount() const
    {
        if(fMasterNode){
            return entries.size();
        } else {
            return entriesCount;
        }
    }

    /// Get the time the last entry was accepted (time in UTC milliseconds)
    int GetLastEntryAccepted() const
    {
        return lastEntryAccepted;
    }

    /// Get the count of the accepted entries
    int GetCountEntriesAccepted() const
    {
        return countEntriesAccepted;
    }

    /// Get the client's transaction count
    int GetMyTransactionCount() const
    {
        return myEntries.size();
    }

    // Set the 'state' value, with some logging and capturing when the state changed
    void UpdateState(unsigned int newState)
    {
        if (fMasterNode && (newState == POOL_STATUS_ERROR || newState == POOL_STATUS_SUCCESS)){
            LogPrintf("CDarksendPool::UpdateState() - Can't set state to ERROR or SUCCESS as a Masternode. \n");
            return;
        }

        LogPrintf("CDarksendPool::UpdateState() == %d | %d \n", state, newState);
        if(state != newState){
            lastTimeChanged = GetTimeMillis();
            if(fMasterNode) {
                RelayStatus(darkSendPool.sessionID, darkSendPool.GetState(), darkSendPool.GetEntriesCount(), MASTERNODE_RESET);
            }
        }
        state = newState;
    }

    /// Get the maximum number of transactions for the pool
    int GetMaxPoolTransactions()
    {
        //if we're on testnet, just use two transactions per merge
        if(Params().NetworkID() == CChainParams::TESTNET || Params().NetworkID() == CChainParams::REGTEST) return POOL_MAX_TRANSACTIONS_TESTNET;

        //use the production amount
        return POOL_MAX_TRANSACTIONS;
    }

    /// Do we have enough users to take entries?
    bool IsSessionReady(){
        return sessionUsers >= GetMaxPoolTransactions();
    }

    /// Are these outputs compatible with other client in the pool?
    bool IsCompatibleWithEntries(std::vector<CTxOut>& vout);

    /// Is this amount compatible with other client in the pool?
    bool IsCompatibleWithSession(int64_t nAmount, CTransaction txCollateral, std::string& strReason);

    /// Passively run Darksend in the background according to the configuration in settings (only for QT)
    bool DoAutomaticDenominating(bool fDryRun=false, bool ready=false);
    bool PrepareDarksendDenominate();

    /// Check for process in Darksend
    void Check();
    void CheckFinalTransaction();
    /// Charge fees to bad actors (Charge clients a fee if they're abusive)
    void ChargeFees();
    /// Rarely charge fees to pay miners
    void ChargeRandomFees();
    void CheckTimeout();
    void CheckForCompleteQueue();
    /// Check to make sure a signature matches an input in the pool
    bool SignatureValid(const CScript& newSig, const CTxIn& newVin);
    /// If the collateral is valid given by a client
    bool IsCollateralValid(const CTransaction& txCollateral);
    /// Add a clients entry to the pool
    bool AddEntry(const std::vector<CTxIn>& newInput, const int64_t& nAmount, const CTransaction& txCollateral, const std::vector<CTxOut>& newOutput, std::string& error);

    /// Add an anonymous output/inputs/sig
    bool AddAnonymousOutput(const CTxOut& out) {return anonTx.AddOutput(out);}
    bool AddAnonymousInput(const CTxIn& in) {return anonTx.AddInput(in);}
    bool AddAnonymousSig(const CTxIn& in) {return anonTx.AddSig(in);}
    bool AddRelaySignature(vector<unsigned char> vchMasternodeRelaySigIn, int nMasternodeBlockHeightIn, std::string strSharedKey) {
        vchMasternodeRelaySig = vchMasternodeRelaySigIn;
        nMasternodeBlockHeight = nMasternodeBlockHeightIn;
        strMasternodeSharedKey = strSharedKey;
        return true;
    }

    /// Add signature to a vin
    bool AddScriptSig(const CTxIn& newVin);
    /// Check that all inputs are signed. (Are all inputs signed?)
    bool SignaturesComplete();
    /// As a client, send a transaction to a Masternode to start the denomination process
    void SendDarksendDenominate(std::vector<CTxIn>& vin, std::vector<CTxOut>& vout, int64_t amount);
    /// Get Masternode updates about the progress of Darksend
    bool StatusUpdate(int newState, int newEntriesCount, int newAccepted, std::string& error, int newSessionID=0);

    /// As a client, check and sign the final transaction
    bool SignFinalTransaction(CTransaction& finalTransactionNew, CNode* node);

    /// Get the last valid block hash for a given modulus
    bool GetLastValidBlockHash(uint256& hash, int mod=1, int nBlockHeight=0);
    /// Process a new block
    void NewBlock();
    void CompletedTransaction(bool error, std::string lastMessageNew);
    void ClearLastMessage();
    /// Used for liquidity providers
    bool SendRandomPaymentToSelf();
    
    /// Split up large inputs or make fee sized inputs
    bool MakeCollateralAmounts();
    bool CreateDenominated(int64_t nTotalValue);
    
    /// Get the denominations for a list of outputs (returns a bitshifted integer)
    int GetDenominations(const std::vector<CTxOut>& vout);
    int GetDenominations(const std::vector<CTxDSOut>& vout);

    void GetDenominationsToString(int nDenom, std::string& strDenom);

    /// Get the denominations for a specific amount of darkcoin.
    int GetDenominationsByAmount(int64_t nAmount, int nDenomTarget=0);
    int GetDenominationsByAmounts(std::vector<int64_t>& vecAmount);


    //
    // Relay Darksend Messages
    //

    void RelayFinalTransaction(const int sessionID, const CTransaction& txNew);
    void RelaySignaturesAnon(std::vector<CTxIn>& vin);
    void RelayInAnon(std::vector<CTxIn>& vin, std::vector<CTxOut>& vout);
    void RelayIn(const std::vector<CTxDSIn>& vin, const int64_t& nAmount, const CTransaction& txCollateral, const std::vector<CTxDSOut>& vout);
    void RelayStatus(const int sessionID, const int newState, const int newEntriesCount, const int newAccepted, const std::string error="");
    void RelayCompletedTransaction(const int sessionID, const bool error, const std::string errorMessage);
};

void ThreadCheckDarkSendPool();

#endif
