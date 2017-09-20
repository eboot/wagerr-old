#ifndef PIVX_ACCUMULATORS_H
#define PIVX_ACCUMULATORS_H

#include "libzerocoin/Zerocoin.h"
#include "primitives/zerocoin.h"
#include "uint256.h"

class CAccumulators
{
public:
    static CAccumulators& getInstance()
    {
        static CAccumulators instance;
        return instance;
    }
private:
    std::map<int, std::unique_ptr<libzerocoin::Accumulator> > mapAccumulators;
    std::map<uint256, int> mapPubCoins;
    std::map<uint32_t, CBigNum> mapAccumulatorValues;

    CAccumulators() { Setup(); }
    void Setup();

public:
    CAccumulators(CAccumulators const&) = delete;
    void operator=(CAccumulators const&) = delete;

    libzerocoin::Accumulator Get(libzerocoin::CoinDenomination denomination);
    void AddPubCoinToAccumulator(libzerocoin::CoinDenomination denomination, libzerocoin::PublicCoin publicCoin);
    void AddAccumulatorChecksum(const CBigNum &bnValue);
    uint32_t GetChecksum(const CBigNum &bnValue);
    CBigNum GetAccumulatorValueFromChecksum(const uint32_t nChecksum);
    bool IntializeWitnessAndAccumulator(const CZerocoinMint &zerocoinSelected, const libzerocoin::PublicCoin &pubcoinSelected, libzerocoin::Accumulator& accumulator, libzerocoin::AccumulatorWitness& witness);
    //CBigNum GetAccumulatorValueFromBlock(CoinDenomination denomination, int nBlockHeight);
    //bool VerifyWitness(CoinDenomination denomination, int nBlockHeight, CBigNum witness);
};


#endif //PIVX_ACCUMULATORS_H
