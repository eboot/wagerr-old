#ifndef PIVX_ZEROCOIN_H
#define PIVX_ZEROCOIN_H
#include "../libzerocoin/bignum.h"

class CZerocoinMint
{
private:
    int denomination;
    int nHeight;
    int id;
    CBigNum value;
    CBigNum randomness;
    CBigNum serialNumber;
    bool isUsed;

public:
    CZerocoinMint()
    {
        SetNull();
    }

    void SetNull()
    {
        isUsed = false;
        randomness = 0;
        value = 0;
        denomination = -1;
        nHeight = -1;
        id = -1;
    }

    //the ZCoin code assumes (denomination, id) is a unique tupple
    std::string ToUniqueString()
    {
        return std::to_string(denomination) + ":" + std::to_string(id);
    }

    CBigNum GetValue() const { return value; }
    void SetValue(CBigNum value){ this->value = value; }
    int GetDenomination() const { return denomination; }
    void SetDenomination(int denomination){ this->denomination = denomination; }
    int GetHeight() const { return nHeight; }
    void SetHeight(int nHeight){ this->nHeight = nHeight; }
    bool IsUsed() const { return this->isUsed; }
    void SetUsed(bool isUsed){ this->isUsed = isUsed; }
    int GetId() const { return id; }
    void SetId(int id){ this->id = id; }
    CBigNum GetRandomness() const{ return randomness; }
    void SetRandomness(CBigNum rand){ this->randomness = rand; }
    CBigNum GetSerialNumber() const { return serialNumber; }
    void SetSerialNumber(CBigNum serial){ this->serialNumber = serial; }

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(isUsed);
        READWRITE(randomness);
        READWRITE(serialNumber);
        READWRITE(value);
        READWRITE(denomination);
        READWRITE(nHeight);
        READWRITE(id);
    };

};

class CZerocoinSpend
{
private:
    CBigNum coinSerial;
    uint256 hashTx;
    CBigNum pubCoin;
    int denomination;
    int id;

public:
    CZerocoinSpend()
    {
        SetNull();
    }

    CZerocoinSpend(CBigNum coinSerial, uint256 hashTx, CBigNum pubCoin, int denomination, int id)
    {
        this->coinSerial = coinSerial;
        this->hashTx = hashTx;
        this->pubCoin = pubCoin;
        this->denomination = denomination;
        this->id = id;
    }

    void SetNull()
    {
        coinSerial = 0;
        hashTx = 0;
        pubCoin = 0;
        denomination = -1;
        id = 0;
    }

    CBigNum GetSerial() const { return coinSerial; }
    uint256 GetTxHash() const { return hashTx; }
    CBigNum GetPubCoin() const { return pubCoin; }
    int GetDenomination() const { return denomination; }
    int GetId() const { return id; }

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(coinSerial);
        READWRITE(hashTx);
        READWRITE(pubCoin);
        READWRITE(denomination);
        READWRITE(id);
    };
};

#endif //PIVX_ZEROCOIN_H
