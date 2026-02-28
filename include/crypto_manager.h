#ifndef CRYPTO_MANAGER_H
#define CRYPTO_MANAGER_H
#include "ciphertext-ser.h"

#include "openfhe.h"

using namespace lbcrypto;

class CryptoContextManager {
private:
    CryptoContext<DCRTPoly> cryptoContext;
    KeyPair<DCRTPoly> keyPair;

public:
    CryptoContextManager();
    ~CryptoContextManager() = default;
    std::string SerializeCiphertext(Ciphertext<DCRTPoly> ciphertext);

    void InitializeBFVContext();

    void GenerateKeys();

    CryptoContext<DCRTPoly> GetContext() const;
    PublicKey<DCRTPoly> GetPublicKey() const;
    PrivateKey<DCRTPoly> GetSecretKey() const;
};

#endif 