#include "../include/crypto_manager.h"
#include "../include/parquet_handler.h"
#include "ciphertext-ser.h"
#include "cryptocontext-ser.h"
#include "scheme/bfvrns/bfvrns-ser.h"
#include <iostream>
#include <vector>

using namespace lbcrypto;

int main() {
    std::cout << "=== Shielded-Parquet FHE Initialization ===" << std::endl;

    CryptoContextManager cryptoManager;
    cryptoManager.InitializeBFVContext();
    cryptoManager.GenerateKeys();
    cryptoManager.GenerateAESKey();

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
              
    std::string blob = cryptoManager.SerializeCiphertext(ciphertext);
    WriteShieldedParquet("salaries.parquet", 101, blob);

    // -----------------------------------------------------------------
    std::cout << "\n--- SIMULATING CLOUD COMPUTATION ---" << std::endl;

    // 1. Read the BLOB from the Parquet file
    std::string loadedBlob = ReadShieldedParquet("salaries.parquet");
    std::cout << "[Cloud] Loaded " << loadedBlob.size() << " bytes from Parquet." << std::endl;

    // 2. Turn bytes back into a Ciphertext math object
    auto loadedCiphertext = cryptoManager.DeserializeCiphertext(loadedBlob);

    // 3. Perform the math on the data loaded from DISK
    auto resultFromDisk = cc->EvalAdd(loadedCiphertext, loadedCiphertext);
    std::cout << "[Cloud] Computation on encrypted Parquet data complete." << std::endl;
    // -----------------------------------------------------------------

    std::cout << "\n--- SIMULATING CLIENT VERIFICATION ---" << std::endl;

    // 4. Decrypt the result (The Client does this locally)
    Plaintext finalResult;
    cc->Decrypt(secretKey, resultFromDisk, &finalResult);
    finalResult->SetLength(mockSalaries.size());

    std::cout << "[Client] Final Decrypted Result from Disk: " << finalResult << std::endl;

    return 0;
}