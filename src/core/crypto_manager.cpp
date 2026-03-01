#include "../../include/crypto_manager.h"
#include "ciphertext-ser.h"
#include "cryptocontext-ser.h"
#include "scheme/bfvrns/bfvrns-ser.h"
#include <iostream>

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

PublicKey<DCRTPoly> CryptoContextManager::GetPublicKey() const {
    return keyPair.publicKey;
}

PrivateKey<DCRTPoly> CryptoContextManager::GetSecretKey() const {
    return keyPair.secretKey;
}