#include "../include/crypto_manager.h"
#include <iostream>
#include <vector>

using namespace lbcrypto;

int main() {
    std::cout << "=== Shielded-Parquet FHE Initialization ===" << std::endl;

    CryptoContextManager cryptoManager;
    cryptoManager.InitializeBFVContext();
    cryptoManager.GenerateKeys();

    auto cc = cryptoManager.GetContext();
    auto publicKey = cryptoManager.GetPublicKey();
    auto secretKey = cryptoManager.GetSecretKey();

    std::vector<int64_t> mockSalaries = {85000, 112000, 95000, 150000};
    std::cout << "\n[Client] Original Salaries: ";
    for (auto salary : mockSalaries) std::cout << salary << " ";
    std::cout << std::endl;

    Plaintext plaintextSalaries = cc->MakePackedPlaintext(mockSalaries);

    auto ciphertext = cc->Encrypt(publicKey, plaintextSalaries);
    std::cout << "[Client] Data Encrypted. Ciphertext size: " 
              << ciphertext->GetElements()[0].GetLength() << " polynomials." << std::endl;

    auto ciphertextDoubled = cc->EvalAdd(ciphertext, ciphertext);
    std::cout << "[Cloud] Executed Homomorphic Addition (Doubling salaries) blindly." << std::endl;

    Plaintext plaintextResult;
    cc->Decrypt(secretKey, ciphertextDoubled, &plaintextResult);
    
    plaintextResult->SetLength(mockSalaries.size()); 

    std::cout << "\n[Client] Decrypted Result: " << plaintextResult << std::endl;

    return 0;
}