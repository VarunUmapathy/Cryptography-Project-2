#include "../../include/crypto_manager.h"
#include "ciphertext-ser.h"
#include "cryptocontext-ser.h"
#include "scheme/bfvrns/bfvrns-ser.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <cstring>
#include <iostream>
#include <fstream>

using namespace lbcrypto;

CryptoContextManager::CryptoContextManager() {}

void CryptoContextManager::InitializeBFVContext() {
    std::cout << "Initializing BFV CryptoContext..." << std::endl;

    CCParams<CryptoContextBFVRNS> parameters;
    
    parameters.SetPlaintextModulus(536903681); 
    
    parameters.SetMultiplicativeDepth(2);      

    cryptoContext = GenCryptoContext(parameters);

    cryptoContext->Enable(PKE);        
    cryptoContext->Enable(KEYSWITCH);   
    cryptoContext->Enable(LEVELEDSHE);

    std::cout << "BFV Context initialized successfully!" << std::endl;
}

void CryptoContextManager::GenerateKeys() {
    std::cout << "Generating Public and Private Keys..." << std::endl;
    
    keyPair = cryptoContext->KeyGen();

    // Generate Relinerarization keys (Essential for BFV math stability)
    cryptoContext->EvalMultKeyGen(keyPair.secretKey);
    
    // Generate Index/Rotation keys (Good practice for SIMD vectors)
    cryptoContext->EvalAtIndexKeyGen(keyPair.secretKey, {1, 2, -1, -2});
    
    std::cout << "Keys generated successfully!" << std::endl;
}

CryptoContext<DCRTPoly> CryptoContextManager::GetContext() const {
    return cryptoContext;
}

std::string CryptoContextManager::SerializeCiphertext(Ciphertext<DCRTPoly> ciphertext) {
    std::stringstream ss;
    // Serializes the ciphertext into a binary stream
    Serial::Serialize(ciphertext, ss, SerType::BINARY);
    return ss.str();
}

Ciphertext<DCRTPoly> CryptoContextManager::DeserializeCiphertext(const std::string& serializedData) {
    std::stringstream ss(serializedData);
    Ciphertext<DCRTPoly> ciphertext;
    
    // Reconstruct the math object from the byte stream
    try {
        Serial::Deserialize(ciphertext, ss, SerType::BINARY);
    } catch (const std::exception& e) {
        std::cerr << "Deserialization Error: " << e.what() << std::endl;
    }
    
    return ciphertext;
}

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

void CryptoContextManager::SaveKeysToDisk(const std::string& path) {
    std::cout << "[Client] Saving keys to secure storage..." << std::endl;
    // Save FHE Context & Secret Key
    Serial::SerializeToFile(path + "cc.bin", cryptoContext, SerType::BINARY);
    Serial::SerializeToFile(path + "secret.bin", keyPair.secretKey, SerType::BINARY);
    
    // Save AES Key
    std::ofstream aesFile(path + "aes.bin", std::ios::binary);
    aesFile.write((char*)aesKey.data(), aesKey.size());
    aesFile.close();
}

void CryptoContextManager::LoadEvaluationKeysFromDisk(const std::string& path) {
    // The Cloud ONLY loads the context to understand the math parameters. 
    // It is physically incapable of loading the secret.bin file here!
    Serial::DeserializeFromFile(path + "cc.bin", cryptoContext, SerType::BINARY);
}

void CryptoContextManager::LoadAllKeysFromDisk(const std::string& path) {
    Serial::DeserializeFromFile(path + "cc.bin", cryptoContext, SerType::BINARY);
    Serial::DeserializeFromFile(path + "secret.bin", keyPair.secretKey, SerType::BINARY);

    // Load AES Key
    aesKey.resize(32);
    std::ifstream aesFile(path + "aes.bin", std::ios::binary);
    aesFile.read((char*)aesKey.data(), 32);
    aesFile.close();
}

PublicKey<DCRTPoly> CryptoContextManager::GetPublicKey() const {
    return keyPair.publicKey;
}

PrivateKey<DCRTPoly> CryptoContextManager::GetSecretKey() const {
    return keyPair.secretKey;
}