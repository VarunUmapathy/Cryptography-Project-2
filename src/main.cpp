#include "../include/crypto_manager.h"
#include "../include/schema_parser.h"
#include "../include/csv_ingester.h"
#include "../include/parquet_handler.h"
#include <iostream>
#include <string>

using namespace lbcrypto;

int main(int argc, char* argv[]) {
    if (argc < 2) return -1;
    std::string command = argv[1];
    CryptoContextManager cryptoManager;

    // We store keys in the same directory Python uses
    std::string key_dir = "temp_data/";

    if (command == "ingest" && argc == 5) {
        std::cout << "[Client] Initializing context and generating keys..." << std::endl;
        cryptoManager.InitializeBFVContext();
        cryptoManager.GenerateKeys();
        cryptoManager.GenerateAESKey();
        cryptoManager.SaveKeysToDisk(key_dir); 

        DatasetSchema schema = ParseSchema(argv[3]);
        IngestCSV(argv[2], argv[4], schema, cryptoManager);
    } 
    else if (command == "compute" && argc == 4) {
        std::cout << "[Cloud] Loading context and Evaluation keys only..." << std::endl;
        cryptoManager.LoadEvaluationKeysFromDisk(key_dir);

        std::cout << "[Cloud] Simulating computation (Doubling BasePay) blindly..." << std::endl;
        ProcessCloudCompute(argv[2], argv[3], cryptoManager);
    } 
    else if (command == "decrypt" && argc == 3) {
        cryptoManager.LoadAllKeysFromDisk(key_dir);
        DecryptShieldedParquet(argv[2], cryptoManager);
    }
    return 0;
}