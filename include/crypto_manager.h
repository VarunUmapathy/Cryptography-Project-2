#pragma once
#include <string>
#include <vector>

class CryptoContextManager {
private:
    std::vector<unsigned char> aesKey;

public:
    CryptoContextManager();

    // AES
    void GenerateAESKey();
    std::string AESEncrypt(const std::string& plaintext) const;
    std::string AESDecrypt(const std::string& ciphertext) const;

};