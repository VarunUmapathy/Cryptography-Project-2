#ifndef CRYPTO_MANAGER_H
#define CRYPTO_MANAGER_H
#include "ciphertext-ser.h"
#include <vector>

#include "openfhe.h"

using namespace lbcrypto;

class CryptoContextManager {
private:
    CryptoContext<DCRTPoly> cryptoContext;
    KeyPair<DCRTPoly> keyPair;
    std::vector<unsigned char> aesKey;

public:
    CryptoContextManager();
    ~CryptoContextManager() = default;
    std::string SerializeCiphertext(Ciphertext<DCRTPoly> ciphertext);
    Ciphertext<DCRTPoly> DeserializeCiphertext(const std::string& serializedData);

    void GenerateAESKey();
    void SaveKeysToDisk(const std::string& path);
    void LoadEvaluationKeysFromDisk(const std::string& path);
    void LoadAllKeysFromDisk(const std::string& path);
    std::string AESEncrypt(const std::string& plaintext) const;

    void InitializeBFVContext();

    void GenerateKeys();

    CryptoContext<DCRTPoly> GetContext() const;
    PublicKey<DCRTPoly> GetPublicKey() const;
    PrivateKey<DCRTPoly> GetSecretKey() const;
};

#endif 