// Copyright (c) 2014-2015 The Dash Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "db.h"
#include "init.h"
#include "activemasternode.h"
#include "masternodeman.h"
#include "masternode-payments.h"
#include "masternode-budget.h"
#include "masternodeconfig.h"
#include "rpcserver.h"
#include "utilmoneystr.h"

#include <boost/lexical_cast.hpp>
#include <fstream>
using namespace json_spirit;
using namespace std;

Value mnbudget(const Array& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "vote-many" && strCommand != "prepare" && strCommand != "submit" && strCommand != "vote" && strCommand != "getvotes" && strCommand != "getinfo" && strCommand != "show" && strCommand != "projection" && strCommand != "check"))
        throw runtime_error(
                "mnbudget \"command\"... ( \"passphrase\" )\n"
                "Vote or show current budgets\n"
                "\nAvailable commands:\n"
                "  prepare            - Prepare proposal for network by signing and creating tx\n"
                "  submit             - Submit proposal for network\n"
                "  vote-many          - Vote on a Dash initiative\n"
                "  vote-alias         - Vote on a Dash initiative\n"
                "  vote               - Vote on a Dash initiative/budget\n"
                "  getvotes           - Show current masternode budgets\n"
                "  getinfo            - Show current masternode budgets\n"
                "  show               - Show all budgets\n"
                "  projection         - Show the projection of which proposals will be paid the next cycle\n"
                "  check              - Scan proposals and remove invalid\n"
                );

    if(strCommand == "prepare")
    {
        int nBlockMin = 0;
        CBlockIndex* pindexPrev = chainActive.Tip();

        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        if (params.size() != 7 && params.size() != 8)
            throw runtime_error("Correct usage of prepare is 'mnbudget prepare proposal-name url payment_count block_start dash_address dash_amount [use_ix(true|false)]'");

        std::string strProposalName = params[1].get_str();
        if(strProposalName.size() > 20)
            return "Invalid proposal name, limit of 20 characters.";

        std::string strURL = params[2].get_str();
        if(strURL.size() > 64)
            return "Invalid url, limit of 64 characters.";

        int nPaymentCount = params[3].get_int();
        if(nPaymentCount < 1)
            return "Invalid payment count, must be more than zero.";

        //set block min
        if(pindexPrev != NULL) nBlockMin = pindexPrev->nHeight - GetBudgetPaymentCycleBlocks()*(nPaymentCount+1);

        int nBlockStart = params[4].get_int();
        if(nBlockStart % GetBudgetPaymentCycleBlocks() != 0){
            int nNext = pindexPrev->nHeight-(pindexPrev->nHeight % GetBudgetPaymentCycleBlocks())+GetBudgetPaymentCycleBlocks();
            return "Invalid block start - must be a budget cycle block. Next valid block: " + boost::lexical_cast<std::string>(nNext);
        }

        int nBlockEnd = nBlockStart + (GetBudgetPaymentCycleBlocks()*nPaymentCount);

        if(nBlockStart < nBlockMin)
            return "Invalid payment count, must be more than current height.";

        if(nBlockEnd < pindexPrev->nHeight)
            return "Invalid ending block, starting block + (payment_cycle*payments) must be more than current height.";

        CBitcoinAddress address(params[5].get_str());
        if (!address.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Dash address");

        // Parse Dash address
        CScript scriptPubKey = GetScriptForDestination(address.Get());

        CAmount nAmount = AmountFromValue(params[6]);

        CPubKey pubKeyMasternode;
        CKey keyMasternode;
        std::string errorMessage;

        if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
            return(" Error upon calling SetKey");

        //*************************************************************************

        CBudgetProposalBroadcast budgetProposalBroadcast(strProposalName, strURL, nPaymentCount, scriptPubKey, nAmount, nBlockStart, 0);
        std::string strCommand = "tx";
        bool useIX = true;
        if (params.size() > 7)
            useIX = (params[7].get_str() == "true" ? true : false);

        if(useIX)
        {
            strCommand = "ix";
        }

        CWalletTx wtx;
        pwalletMain->GetBudgetSystemCollateralTX(wtx, budgetProposalBroadcast.GetHash(), useIX);

        // make our change address
        CReserveKey reservekey(pwalletMain);
        //send the tx to the network
        pwalletMain->CommitTransaction(wtx, reservekey, strCommand);

        return wtx.GetHash().ToString().c_str();

    }

    if(strCommand == "submit")
    {
        int nBlockMin = 0;
        CBlockIndex* pindexPrev = chainActive.Tip();

        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        if (params.size() != 8)
            throw runtime_error("Correct usage of submit is 'mnbudget submit proposal-name url payment_count block_start dash_address dash_amount fee_tx'");

        // Check these inputs the same way we check the vote commands:
        // **********************************************************

        std::string strProposalName = params[1].get_str();
        if(strProposalName.size() > 20)
            return "Invalid proposal name, limit of 20 characters.";

        std::string strURL = params[2].get_str();
        if(strURL.size() > 64)
            return "Invalid url, limit of 64 characters.";

        int nPaymentCount = params[3].get_int();
        if(nPaymentCount < 1)
            return "Invalid payment count, must be more than zero.";

        //set block min
        if(pindexPrev != NULL) nBlockMin = pindexPrev->nHeight - GetBudgetPaymentCycleBlocks()*(nPaymentCount+1);

        int nBlockStart = params[4].get_int();
        if(nBlockStart % GetBudgetPaymentCycleBlocks() != 0){
            int nNext = pindexPrev->nHeight-(pindexPrev->nHeight % GetBudgetPaymentCycleBlocks())+GetBudgetPaymentCycleBlocks();
            return "Invalid block start - must be a budget cycle block. Next valid block: " + boost::lexical_cast<std::string>(nNext);
        }

        int nBlockEnd = nBlockStart + (GetBudgetPaymentCycleBlocks()*nPaymentCount);

        if(nBlockStart < nBlockMin)
            return "Invalid payment count, must be more than current height.";

        if(nBlockEnd < pindexPrev->nHeight)
            return "Invalid ending block, starting block + (payment_cycle*payments) must be more than current height.";

        CBitcoinAddress address(params[5].get_str());
        if (!address.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Dash address");

        // Parse Dash address
        CScript scriptPubKey = GetScriptForDestination(address.Get());

        CAmount nAmount = AmountFromValue(params[6]);

        CPubKey pubKeyMasternode;
        CKey keyMasternode;
        std::string errorMessage;

        if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
            return(" Error upon calling SetKey");

        uint256 hash = ParseHashV(params[7], "parameter 1");

        //create the proposal incase we're the first to make it
        CBudgetProposalBroadcast budgetProposalBroadcast(strProposalName, strURL, nPaymentCount, scriptPubKey, nAmount, nBlockStart, hash);

        std::string strError = "";
        if(!IsBudgetCollateralValid(hash ,strError)){
            return "Proposal FeeTX is not valid - " + hash.ToString() + " - " + strError;
        }

        if(!budgetProposalBroadcast.IsValid(strError)){
            return "Proposal is not valid - " + budgetProposalBroadcast.GetHash().ToString() + " - " + strError;
        }

        mapSeenMasternodeBudgetProposals.insert(make_pair(budgetProposalBroadcast.GetHash(), budgetProposalBroadcast));
        budgetProposalBroadcast.Relay();
        budget.AddProposal(budgetProposalBroadcast);

        return budgetProposalBroadcast.GetHash().ToString().c_str();

    }

    if(strCommand == "vote-many")
    {
        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        if (params.size() != 3)
            throw runtime_error("Correct usage of vote-many is 'mnbudget vote-many proposal-hash yes|no'");

        uint256 hash = ParseHashV(params[1], "parameter 1");
        std::string strVote = params[2].get_str().c_str();

        if(strVote != "yes" && strVote != "no") return "You can only vote 'yes' or 'no'";
        int nVote = VOTE_ABSTAIN;
        if(strVote == "yes") nVote = VOTE_YES;
        if(strVote == "no") nVote = VOTE_NO;

        int success = 0;
        int failed = 0;

        Object resultObj;

        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
            std::string errorMessage;
            std::vector<unsigned char> vchMasterNodeSignature;
            std::string strMasterNodeSignMessage;

            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            if(!darkSendSigner.SetKey(mne.getPrivKey(), errorMessage, keyMasternode, pubKeyMasternode)){
                printf(" Error upon calling SetKey for %s\n", mne.getAlias().c_str());
                failed++;
                continue;
            }

            CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
            if(pmn == NULL)
            {
                printf("Can't find masternode by pubkey for %s\n", mne.getAlias().c_str());
                failed++;
                continue;
            }

            CBudgetVote vote(pmn->vin, hash, nVote);
            if(!vote.Sign(keyMasternode, pubKeyMasternode)){
                return "Failure to sign.";
            }

            mapSeenMasternodeBudgetVotes.insert(make_pair(vote.GetHash(), vote));
            vote.Relay();
            budget.UpdateProposal(vote, NULL);

            success++;
        }

        return("Voted successfully " + boost::lexical_cast<std::string>(success) + " time(s) and failed " + boost::lexical_cast<std::string>(failed) + " time(s).");
    }

    if(strCommand == "vote")
    {
        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        if (params.size() != 3)
            throw runtime_error("Correct usage of vote is 'mnbudget vote proposal-hash yes|no'");

        uint256 hash = ParseHashV(params[1], "parameter 1");
        std::string strVote = params[2].get_str().c_str();

        if(strVote != "yes" && strVote != "no") return "You can only vote 'yes' or 'no'";
        int nVote = VOTE_ABSTAIN;
        if(strVote == "yes") nVote = VOTE_YES;
        if(strVote == "no") nVote = VOTE_NO;

        CPubKey pubKeyMasternode;
        CKey keyMasternode;
        std::string errorMessage;

        if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
            return(" Error upon calling SetKey");

        CBudgetVote vote(activeMasternode.vin, hash, nVote);
        if(!vote.Sign(keyMasternode, pubKeyMasternode)){
            return "Failure to sign.";
        }

        mapSeenMasternodeBudgetVotes.insert(make_pair(vote.GetHash(), vote));
        vote.Relay();
        budget.UpdateProposal(vote, NULL);
    }

    if(strCommand == "projection")
    {
        Object resultObj;
        int64_t nTotalAllotted = 0;

        std::vector<CBudgetProposal*> winningProps = budget.GetBudget();
        BOOST_FOREACH(CBudgetProposal* pbudgetProposal, winningProps)
        {
            nTotalAllotted += pbudgetProposal->GetAllotted();

            CTxDestination address1;
            ExtractDestination(pbudgetProposal->GetPayee(), address1);
            CBitcoinAddress address2(address1);

            Object bObj;
            bObj.push_back(Pair("URL",  pbudgetProposal->GetURL()));
            bObj.push_back(Pair("Hash",  pbudgetProposal->GetHash().ToString().c_str()));
            bObj.push_back(Pair("BlockStart",  (int64_t)pbudgetProposal->GetBlockStart()));
            bObj.push_back(Pair("BlockEnd",    (int64_t)pbudgetProposal->GetBlockEnd()));
            bObj.push_back(Pair("TotalPaymentCount",  (int64_t)pbudgetProposal->GetTotalPaymentCount()));
            bObj.push_back(Pair("RemainingPaymentCount",  (int64_t)pbudgetProposal->GetRemainingPaymentCount()));
            bObj.push_back(Pair("PaymentAddress",   address2.ToString().c_str()));
            bObj.push_back(Pair("Ratio",  pbudgetProposal->GetRatio()));
            bObj.push_back(Pair("Yeas",  (int64_t)pbudgetProposal->GetYeas()));
            bObj.push_back(Pair("Nays",  (int64_t)pbudgetProposal->GetNays()));
            bObj.push_back(Pair("Abstains",  (int64_t)pbudgetProposal->GetAbstains()));
            bObj.push_back(Pair("Alloted",  (int64_t)pbudgetProposal->GetAllotted()));
            bObj.push_back(Pair("TotalBudgetAlloted",  nTotalAllotted));

            std::string strError = "";
            bObj.push_back(Pair("IsValid",  pbudgetProposal->IsValid(strError)));

            resultObj.push_back(Pair(pbudgetProposal->GetName().c_str(), bObj));
        }

        return resultObj;
    }

    if(strCommand == "show")
    {
        Object resultObj;
        int64_t nTotalAllotted = 0;

        std::vector<CBudgetProposal*> winningProps = budget.GetAllProposals();
        BOOST_FOREACH(CBudgetProposal* pbudgetProposal, winningProps)
        {
            nTotalAllotted += pbudgetProposal->GetAllotted();

            CTxDestination address1;
            ExtractDestination(pbudgetProposal->GetPayee(), address1);
            CBitcoinAddress address2(address1);

            Object bObj;
            bObj.push_back(Pair("URL",  pbudgetProposal->GetURL()));
            bObj.push_back(Pair("Hash",  pbudgetProposal->GetHash().ToString().c_str()));
            bObj.push_back(Pair("BlockStart",  (int64_t)pbudgetProposal->GetBlockStart()));
            bObj.push_back(Pair("BlockEnd",    (int64_t)pbudgetProposal->GetBlockEnd()));
            bObj.push_back(Pair("TotalPaymentCount",  (int64_t)pbudgetProposal->GetTotalPaymentCount()));
            bObj.push_back(Pair("RemainingPaymentCount",  (int64_t)pbudgetProposal->GetRemainingPaymentCount()));
            bObj.push_back(Pair("PaymentAddress",   address2.ToString().c_str()));
            bObj.push_back(Pair("Ratio",  pbudgetProposal->GetRatio()));
            bObj.push_back(Pair("Yeas",  (int64_t)pbudgetProposal->GetYeas()));
            bObj.push_back(Pair("Nays",  (int64_t)pbudgetProposal->GetNays()));
            bObj.push_back(Pair("Abstains",  (int64_t)pbudgetProposal->GetAbstains()));
            bObj.push_back(Pair("Amount",  (int64_t)pbudgetProposal->GetAmount()));

            std::string strError = "";
            bObj.push_back(Pair("IsValid",  pbudgetProposal->IsValid(strError)));


            resultObj.push_back(Pair(pbudgetProposal->GetName().c_str(), bObj));
        }

        return resultObj;
    }

    if(strCommand == "getinfo")
    {
        if (params.size() != 2)
            throw runtime_error("Correct usage of getinfo is 'mnbudget getinfo profilename'");

        std::string strProposalName = params[1].get_str();

        CBudgetProposal* pbudgetProposal = budget.FindProposal(strProposalName);

        if(pbudgetProposal == NULL) return "Unknown proposal name";

        CTxDestination address1;
        ExtractDestination(pbudgetProposal->GetPayee(), address1);
        CBitcoinAddress address2(address1);

        Object obj;
        obj.push_back(Pair("Name",  pbudgetProposal->GetName().c_str()));
        obj.push_back(Pair("Hash",  pbudgetProposal->GetHash().ToString().c_str()));
        obj.push_back(Pair("URL",  pbudgetProposal->GetURL().c_str()));
        obj.push_back(Pair("BlockStart",  (int64_t)pbudgetProposal->GetBlockStart()));
        obj.push_back(Pair("BlockEnd",    (int64_t)pbudgetProposal->GetBlockEnd()));
        obj.push_back(Pair("TotalPaymentCount",  (int64_t)pbudgetProposal->GetTotalPaymentCount()));
        obj.push_back(Pair("RemainingPaymentCount",  (int64_t)pbudgetProposal->GetRemainingPaymentCount()));
        obj.push_back(Pair("PaymentAddress",   address2.ToString().c_str()));
        obj.push_back(Pair("Ratio",  pbudgetProposal->GetRatio()));
        obj.push_back(Pair("Yeas",  (int64_t)pbudgetProposal->GetYeas()));
        obj.push_back(Pair("Nays",  (int64_t)pbudgetProposal->GetNays()));
        obj.push_back(Pair("Abstains",  (int64_t)pbudgetProposal->GetAbstains()));
        obj.push_back(Pair("Alloted",  (int64_t)pbudgetProposal->GetAllotted()));

        std::string strError = "";
        obj.push_back(Pair("IsValid",  pbudgetProposal->IsValid(strError)));

        return obj;
    }

    if(strCommand == "getvotes")
    {
        if (params.size() != 2)
            throw runtime_error("Correct usage of getvotes is 'mnbudget getinfo profilename'");

        std::string strProposalName = params[1].get_str();

        Object obj;

        CBudgetProposal* pbudgetProposal = budget.FindProposal(strProposalName);

        if(pbudgetProposal == NULL) return "Unknown proposal name";

        std::map<uint256, CBudgetVote>::iterator it = pbudgetProposal->mapVotes.begin();
        while(it != pbudgetProposal->mapVotes.end()){
            obj.push_back(Pair((*it).second.vin.prevout.ToStringShort().c_str(),  (*it).second.GetVoteString().c_str()));
            it++;
        }


        return obj;
    }

    if(strCommand == "check")
    {
        budget.CheckAndRemove();

        return "Success";
    }

    return Value::null;
}

Value mnfinalbudget(const Array& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "suggest" && strCommand != "vote-many" && strCommand != "vote" && strCommand != "show"))
        throw runtime_error(
                "mnbudget \"command\"... ( \"passphrase\" )\n"
                "Vote or show current budgets\n"
                "\nAvailable commands:\n"
                "  suggest     - Suggest a budget to be paid\n"
                "  vote-many   - Vote on a finalized budget\n"
                "  vote        - Vote on a finalized budget\n"
                "  show        - Show existing finalized budgets\n"
                );

    if(strCommand == "suggest")
    {
        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        CBlockIndex* pindexPrev = chainActive.Tip();
        if(!pindexPrev)
            return "Must be synced to suggest";

        if (params.size() < 3)
            throw runtime_error("Correct usage of suggest is 'mnfinalbudget suggest BUDGET_NAME PROPNAME [PROP2 PROP3 PROP4]'");

        std::string strBudgetName = params[1].get_str();
        if(strBudgetName.size() > 20)
            return "Invalid budget name, limit of 20 characters.";

        int nBlockStart = pindexPrev->nHeight-(pindexPrev->nHeight % GetBudgetPaymentCycleBlocks())+GetBudgetPaymentCycleBlocks();

        std::vector<CTxBudgetPayment> vecPayments;
        for(int i = 2; i < (int)params.size(); i++)
        {
            std::string strHash = params[i].get_str();
            uint256 hash(strHash);
            CBudgetProposal* pbudgetProposal = budget.FindProposal(hash);
            if(!pbudgetProposal){
                return "Invalid proposal " + strHash + ". Please check the proposal hash";
            } else {
                CTxBudgetPayment out;
                out.nProposalHash = hash;
                out.payee = pbudgetProposal->GetPayee();
                out.nAmount = pbudgetProposal->GetAmount();
                vecPayments.push_back(out);
            }
        }

        CPubKey pubKeyMasternode;
        CKey keyMasternode;
        std::string errorMessage;

        if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
            return(" Error upon calling SetKey");

        //create transaction
        uint256 hash = 0;

        //create the proposal incase we're the first to make it
        CFinalizedBudgetBroadcast finalizedBudgetBroadcast(strBudgetName, nBlockStart, vecPayments, hash);

        if(!finalizedBudgetBroadcast.IsValid())
            return "Invalid finalized budget broadcast (are all the hashes correct?)";

        mapSeenFinalizedBudgets.insert(make_pair(finalizedBudgetBroadcast.GetHash(), finalizedBudgetBroadcast));
        finalizedBudgetBroadcast.Relay();
        budget.AddFinalizedBudget(finalizedBudgetBroadcast);

        CFinalizedBudgetVote vote(activeMasternode.vin, finalizedBudgetBroadcast.GetHash());
        if(!vote.Sign(keyMasternode, pubKeyMasternode)){
            return "Failure to sign.";
        }

        mapSeenFinalizedBudgetVotes.insert(make_pair(vote.GetHash(), vote));
        vote.Relay();
        budget.UpdateFinalizedBudget(vote, NULL);

        return "success";

    }

    if(strCommand == "vote-many")
    {
        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        if (params.size() != 2)
            throw runtime_error("Correct usage of vote-many is 'mnfinalbudget vote-many BUDGET_HASH'");

        std::string strHash = params[1].get_str();
        uint256 hash(strHash);

        int success = 0;
        int failed = 0;

        Object resultObj;

        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
            std::string errorMessage;
            std::vector<unsigned char> vchMasterNodeSignature;
            std::string strMasterNodeSignMessage;

            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;
            CPubKey pubKeyMasternode;
            CKey keyMasternode;

            if(!darkSendSigner.SetKey(mne.getPrivKey(), errorMessage, keyMasternode, pubKeyMasternode)){
                printf(" Error upon calling SetKey for %s\n", mne.getAlias().c_str());
                failed++;
                continue;
            }

            CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
            if(pmn == NULL)
            {
                printf("Can't find masternode by pubkey for %s\n", mne.getAlias().c_str());
                failed++;
                continue;
            }


            CFinalizedBudgetVote vote(pmn->vin, hash);
            if(!vote.Sign(keyMasternode, pubKeyMasternode)){
                failed++;
                continue;
            }

            mapSeenFinalizedBudgetVotes.insert(make_pair(vote.GetHash(), vote));
            vote.Relay();
            budget.UpdateFinalizedBudget(vote, NULL);

            success++;
        }

        return("Voted successfully " + boost::lexical_cast<std::string>(success) + " time(s) and failed " + boost::lexical_cast<std::string>(failed) + " time(s).");
    }

    if(strCommand == "vote")
    {
        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        if (params.size() != 2)
            throw runtime_error("Correct usage of vote is 'mnfinalbudget vote BUDGET_HASH'");

        std::string strHash = params[1].get_str();
        uint256 hash(strHash);

        CPubKey pubKeyMasternode;
        CKey keyMasternode;
        std::string errorMessage;

        if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
            return(" Error upon calling SetKey");

        CFinalizedBudgetVote vote(activeMasternode.vin, hash);
        if(!vote.Sign(keyMasternode, pubKeyMasternode)){
            return "Failure to sign.";
        }

        mapSeenFinalizedBudgetVotes.insert(make_pair(vote.GetHash(), vote));
        vote.Relay();
        budget.UpdateFinalizedBudget(vote, NULL);

        return "success";
    }

    if(strCommand == "show")
    {
        Object resultObj;

        std::vector<CFinalizedBudget*> winningFbs = budget.GetFinalizedBudgets();
        BOOST_FOREACH(CFinalizedBudget* finalizedBudget, winningFbs)
        {
            Object bObj;
            bObj.push_back(Pair("FeeTX",  finalizedBudget->nFeeTXHash.ToString().c_str()));
            bObj.push_back(Pair("Hash",  finalizedBudget->GetHash().ToString().c_str()));
            bObj.push_back(Pair("BlockStart",  (int64_t)finalizedBudget->GetBlockStart()));
            bObj.push_back(Pair("BlockEnd",    (int64_t)finalizedBudget->GetBlockEnd()));
            bObj.push_back(Pair("Proposals",  finalizedBudget->GetProposals().c_str()));
            bObj.push_back(Pair("VoteCount",  (int64_t)finalizedBudget->GetVoteCount()));
            bObj.push_back(Pair("Status",  finalizedBudget->GetStatus().c_str()));
            resultObj.push_back(Pair(finalizedBudget->GetName().c_str(), bObj));
        }

        return resultObj;

    }

    return Value::null;
}
