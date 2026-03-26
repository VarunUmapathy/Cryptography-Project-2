#include "../../include/crypto_manager.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <cstring>
#include <iostream>
#include <fstream>

CryptoContextManager::CryptoContextManager() {}

void CryptoContextManager::GenerateAESKey() {
    std::cout << "Generating AES-256 Key..." << std::endl;
    aesKey.resize(32); // 32 bytes = 256 bits
    // Use OpenSSL's cryptographically secure random number generator
    RAND_bytes(aesKey.data(), 32); 
    std::cout << "AES Key generated successfully!" << std::endl;
}

std::string CryptoContextManager::AESEncrypt(const std::string& plaintext) const {
    if (aesKey.empty()) return "";

    // 1. Generate a random 12-byte IV (Nonce)
    unsigned char iv[12];
    RAND_bytes(iv, sizeof(iv));

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, aesKey.data(), iv);

    std::vector<unsigned char> ciphertext(plaintext.length());
    int len;
    int ciphertext_len;

    // 2. Encrypt the plaintext
    EVP_EncryptUpdate(ctx, ciphertext.data(), &len, 
                      (const unsigned char*)plaintext.data(), plaintext.length());
    ciphertext_len = len;

    // 3. Finalize and get the Authentication Tag
    EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
    ciphertext_len += len;

    unsigned char tag[16];
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);
    EVP_CIPHER_CTX_free(ctx);

    // 4. Pack everything into one binary string: [IV(12)][CIPHERTEXT(N)][TAG(16)]
    std::string result;
    result.reserve(sizeof(iv) + ciphertext_len + sizeof(tag));
    result.append((char*)iv, sizeof(iv));
    result.append((char*)ciphertext.data(), ciphertext_len);
    result.append((char*)tag, sizeof(tag));

    return result;
}

std::string CryptoContextManager::AESDecrypt(const std::string& packed_ciphertext) const {
    if (aesKey.empty() || packed_ciphertext.length() < 28) return "[AES_DECRYPTION_FAILED]";

    unsigned char iv[12];
    unsigned char tag[16];
    int ciphertext_len = packed_ciphertext.length() - 28;
    std::vector<unsigned char> ciphertext(ciphertext_len);
    std::memcpy(iv, packed_ciphertext.data(), 12);
    std::memcpy(ciphertext.data(), packed_ciphertext.data() + 12, ciphertext_len);
    std::memcpy(tag, packed_ciphertext.data() + 12 + ciphertext_len, 16);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, aesKey.data(), iv);

    std::vector<unsigned char> plaintext(ciphertext_len);
    int len;
    int plaintext_len;

    EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext_len);
    plaintext_len = len;

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag);
    int ret = EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len);
    EVP_CIPHER_CTX_free(ctx);

    if (ret > 0) {
        plaintext_len += len;
        return std::string((char*)plaintext.data(), plaintext_len);
    } else {
        return "[AUTH_TAG_VERIFICATION_FAILED]";
    }
}