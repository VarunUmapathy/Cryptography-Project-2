#include "../../include/crypto_manager.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include <fstream>
#include <iostream>
#include <cstring>

CryptoContextManager::CryptoContextManager() {}


// ================= KEY MANAGEMENT =================

void CryptoContextManager::GenerateAESKey() {
    aesKey.resize(32); // AES-256
    RAND_bytes(aesKey.data(), 32);
}

void CryptoContextManager::SaveAESKey(const std::string& filename) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to open key file for writing\n";
        return;
    }
    out.write((char*)aesKey.data(), aesKey.size());
}

void CryptoContextManager::LoadAESKey(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open key file for reading\n";
        return;
    }

    aesKey.resize(32);
    in.read((char*)aesKey.data(), 32);

    if (in.gcount() != 32) {
        aesKey.clear();
        std::cerr << "Invalid key file\n";
    }
}


// ================= AES-GCM ENCRYPT =================

std::string CryptoContextManager::AESEncrypt(const std::string& plaintext) const {
    if (aesKey.size() != 32) return "";

    unsigned char iv[12];
    RAND_bytes(iv, sizeof(iv));

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, aesKey.data(), iv);

    std::vector<unsigned char> ciphertext(plaintext.size());
    int len = 0, ciphertext_len = 0;

    EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
        (const unsigned char*)plaintext.data(), plaintext.size());
    ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
    ciphertext_len += len;

    unsigned char tag[16];
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);

    EVP_CIPHER_CTX_free(ctx);

    // Pack: IV + ciphertext + tag
    std::string binary;
    binary.append((char*)iv, 12);
    binary.append((char*)ciphertext.data(), ciphertext_len);
    binary.append((char*)tag, 16);

    return Base64Encode(binary);
}


// ================= AES-GCM DECRYPT =================

std::string CryptoContextManager::AESDecrypt(const std::string& input) const {
    if (aesKey.size() != 32){
        //std::cerr << "[ERROR] Key size: " << aesKey.size() << std::endl;
        return "[AES_DECRYPTION_FAILED]";
    }
    //std::cout << "[DECRYPT] Input(Base64): " << input << std::endl << std::flush;

    std::string packed = Base64Decode(input);

    //std::cout << "[DECRYPT] Decoded size: " << packed.size() << std::endl << std::flush;

    if (packed.size() < 28){
        //std::cerr << "[ERROR] Packed size: " << packed.size() << std::endl;
        return "[AES_DECRYPTION_FAILED]";
    }

    unsigned char iv[12];
    unsigned char tag[16];

    int ciphertext_len = packed.size() - 28;
    std::vector<unsigned char> ciphertext(ciphertext_len);

    std::memcpy(iv, packed.data(), 12);
    std::memcpy(ciphertext.data(), packed.data() + 12, ciphertext_len);
    std::memcpy(tag, packed.data() + 12 + ciphertext_len, 16);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, aesKey.data(), iv);

    std::vector<unsigned char> plaintext(ciphertext_len);
    int len = 0, plaintext_len = 0;

    EVP_DecryptUpdate(ctx, plaintext.data(), &len,
        ciphertext.data(), ciphertext_len);
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


// ================= BASE64 =================

std::string CryptoContextManager::Base64Encode(const std::string& input) const {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    BIO_write(bio, input.data(), input.size());
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);

    return result;
}

std::string CryptoContextManager::Base64Decode(const std::string& input) const {
    BIO *bio, *b64;
    std::vector<char> buffer(input.size());

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf(input.data(), input.size());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    int decodedLen = BIO_read(bio, buffer.data(), input.size());
    BIO_free_all(bio);

    if (decodedLen < 0) return "";

    return std::string(buffer.data(), decodedLen);
}