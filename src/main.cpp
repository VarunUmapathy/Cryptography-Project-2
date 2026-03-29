#include "../include/crypto_manager.h"
#include "../include/schema_parser.h"
#include "../include/csv_ingester.h"
#include "../include/parquet_handler.h"
#include "../include/fhe_engine.h"

#include <iostream>
#include <string>

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

    CryptoContextManager cryptoManager;

    /* =========================
       INGEST PHASE
       ========================= */
    if (command == "ingest" && argc == 5) {
        std::string csvFile = argv[2];
        std::string schemaFile = argv[3];
        std::string outParquet = argv[4];

        std::cout << "[Client] Initializing...\n";

        fhe->Initialize();
        fhe->GenerateKeys();

        cryptoManager.GenerateAESKey();
        cryptoManager.SaveAESKey(key_dir+"aes.key");
        //cryptoManager.SaveKeysToDisk(key_dir);
        fhe->SaveKeys(key_dir);
        DatasetSchema schema = ParseSchema(schemaFile);

        IngestCSV(csvFile, outParquet, schema, cryptoManager, fhe);

        std::cout << "[Client] Ingestion complete\n";
    }

    /* =========================
       CLOUD COMPUTE
       ========================= */
    else if (command == "compute" && argc == 4) {
        std::string inFile = argv[2];
        std::string outFile = argv[3];

        std::cout << "[Cloud] Loading evaluation keys...\n";
        cryptoManager.LoadAESKey(key_dir+"aes.key");
        fhe->LoadEvalKeys(key_dir);

        ProcessCloudCompute(inFile, outFile, cryptoManager, fhe);

        std::cout << "[Cloud] Compute complete\n";
    }

    /* =========================
       DECRYPT PHASE
       ========================= */
    else if (command == "decrypt" && argc == 3) {
        std::string inFile = argv[2];

        std::cout << "[Client] Loading keys...\n";

        //cryptoManager.LoadAllKeysFromDisk(key_dir);
        cryptoManager.LoadAESKey(key_dir+"aes.key");
        fhe->LoadAllKeys(key_dir);

        DecryptShieldedParquet(inFile, cryptoManager, fhe);
    }

    else {
        std::cerr << "Invalid command or arguments\n";
    }

    delete fhe;
    return 0;
}