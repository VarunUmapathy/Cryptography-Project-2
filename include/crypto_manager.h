#ifndef CRYPTO_MANAGER_H
#define CRYPTO_MANAGER_H

#include <string>
#include <vector>

class CryptoContextManager {
private:
    std::vector<unsigned char> aesKey;

    std::string Base64Encode(const std::string& input) const;
    std::string Base64Decode(const std::string& input) const;

public:
    CryptoContextManager();

    void GenerateAESKey();
    void SaveAESKey(const std::string& filename);
    void LoadAESKey(const std::string& filename);

    std::string AESEncrypt(const std::string& plaintext) const;
    std::string AESDecrypt(const std::string& ciphertext) const;
};

#endif