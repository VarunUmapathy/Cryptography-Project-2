#include "../../include/crypto_manager.h"
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

    cryptoContext->EvalMultKeyGen(keyPair.secretKey);
    
    std::cout << "Keys generated successfully!" << std::endl;
}

CryptoContext<DCRTPoly> CryptoContextManager::GetContext() const {
    return cryptoContext;
}

PublicKey<DCRTPoly> CryptoContextManager::GetPublicKey() const {
    return keyPair.publicKey;
}

PrivateKey<DCRTPoly> CryptoContextManager::GetSecretKey() const {
    return keyPair.secretKey;
}