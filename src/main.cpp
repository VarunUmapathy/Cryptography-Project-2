#include "../include/crypto_manager.h"
#include "../include/schema_parser.h"
#include "../include/csv_ingester.h"
#include "../include/parquet_handler.h"
#include "../include/fhe_engine.h"
#include "../include/timer.h"

#include <iostream>
#include <string>
#include <filesystem>

/* =========================
   MAIN
   ========================= */
int main(int argc, char* argv[]) {

    FHEEngine* fhe = CreateFHEEngine();

    if (argc < 2) {
        std::cerr << "Usage:\n";
        std::cerr << "  ingest <csv> <schema> <out_parquet>\n";
        std::cerr << "  compute <in_parquet> <out_parquet>\n";
        std::cerr << "  decrypt <parquet>\n";
        return -1;
    }

    std::string command = argv[1];
    std::string key_dir = "temp_data/";
    std::string fhe_key_file = key_dir + "fhe.keys";
    std::string aes_key_file = key_dir + "aes.key";

    // Ensure directory exists
    std::filesystem::create_directories(key_dir);

    CryptoContextManager cryptoManager;

    /* =========================
       INGEST (CLIENT)
       ========================= */
    if (command == "ingest" && argc == 5) {
        std::string csvFile = argv[2];
        std::string schemaFile = argv[3];
        std::string outParquet = argv[4];

        std::cout << "[Client] Initializing...\n";

        fhe->Initialize();
        fhe->GenerateKeys();
        fhe->SaveKeys(fhe_key_file);

        cryptoManager.GenerateAESKey();
        cryptoManager.SaveAESKey(aes_key_file);

        DatasetSchema schema = ParseSchema(schemaFile);

        IngestCSV(csvFile, outParquet, schema, cryptoManager, fhe);

        std::cout << "[Client] Ingestion complete\n";
        auto original_size = std::filesystem::file_size(csvFile);
        auto encrypted_size = std::filesystem::file_size(outParquet);

        std::cout << "\n--- STORAGE METRICS ---\n";
        std::cout << "Original CSV: " << original_size << " bytes\n";
        std::cout << "Encrypted Parquet: " << encrypted_size << " bytes\n";
        std::cout << "Overhead: " << (double)encrypted_size / original_size << "x\n";
    }
    
    /* =========================
       COMPUTE (CLOUD)
       ========================= */
    else if (command == "compute" && argc == 4) {
        std::string inFile = argv[2];
        std::string outFile = argv[3];

        std::cout << "[Cloud] Loading evaluation keys...\n";

        cryptoManager.LoadAESKey(aes_key_file);

        // ⚠️ Only load eval keys (simulated)
        fhe->LoadEvalKeys(fhe_key_file);

        ProcessCloudCompute(inFile, outFile, cryptoManager, fhe);

        std::cout << "[Cloud] Compute complete\n";
    }

    /* =========================
       DECRYPT (CLIENT)
       ========================= */
    else if (command == "decrypt" && argc == 3) {
        std::string inFile = argv[2];

        std::cout << "[Client] Loading keys...\n";

        cryptoManager.LoadAESKey(aes_key_file);

        // ✔ Load ALL keys (including secret)
        fhe->LoadAllKeys(fhe_key_file);

        DecryptShieldedParquet(inFile, cryptoManager, fhe);
    }

    else {
        std::cerr << "Invalid command or arguments\n";
    }

    delete fhe;
    return 0;
}