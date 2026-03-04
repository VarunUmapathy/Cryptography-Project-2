#include "../include/crypto_manager.h"
#include "../include/schema_parser.h"
#include "../include/csv_ingester.h"
#include <iostream>
#include <string>

using namespace lbcrypto;

void PrintUsage(const char* progName) {
    std::cout << "=== Shielded-Parquet Hybrid Engine ===\n"
              << "Usage: " << progName << " <command> [args...]\n\n"
              << "Commands:\n"
              << "  ingest  <csv_file> <schema_file> <out_parquet>\n"
              << "  compute <in_parquet> <out_parquet>\n"
              << "  decrypt <in_parquet>\n";
}

int main(int argc, char* argv[]) {
    // 1. Ensure a command was provided
    if (argc < 2) {
        PrintUsage(argv[0]);
        return -1;
    }

    std::string command = argv[1];
    CryptoContextManager cryptoManager;

    // ==========================================
    // COMMAND 1: INGEST (Client Side)
    // ==========================================
    if (command == "ingest") {
        if (argc != 5) {
            std::cerr << "Usage: ingest <csv_file> <schema_file> <out_parquet>" << std::endl;
            return -1;
        }
        std::string csvFile = argv[2];
        std::string schemaFile = argv[3];
        std::string outFile = argv[4];

        std::cout << "[Client] Initializing new context and generating keys..." << std::endl;
        cryptoManager.InitializeBFVContext();
        cryptoManager.GenerateKeys();
        cryptoManager.GenerateAESKey();
        
        // TODO: Save keys to a local folder so the next command can use them!
        // cryptoManager.SaveKeysToDisk("keys/"); 

        std::cout << "[Client] Loading encryption schema..." << std::endl;
        DatasetSchema schema = ParseSchema(schemaFile);
        if (schema.columns.empty()) return -1;

        std::cout << "[Client] Starting CSV ingestion..." << std::endl;
        IngestCSV(csvFile, outFile, schema, cryptoManager);
    } 
    
    // ==========================================
    // COMMAND 2: COMPUTE (Cloud Side)
    // ==========================================
    else if (command == "compute") {
        if (argc != 4) {
            std::cerr << "Usage: compute <in_parquet> <out_parquet>" << std::endl;
            return -1;
        }
        std::string inFile = argv[2];
        std::string outFile = argv[3];

        std::cout << "[Cloud] Loading context and Evaluation keys only..." << std::endl;
        // TODO: cryptoManager.LoadEvaluationKeysFromDisk("keys/");

        std::cout << "[Cloud] Simulating computation (Adding a bonus) on " << inFile << "..." << std::endl;
        // TODO: Read Parquet, EvalAdd the FHE columns, write updated Parquet to outFile
    } 
    
    // ==========================================
    // COMMAND 3: DECRYPT (Client Side)
    // ==========================================
    else if (command == "decrypt") {
        if (argc != 3) {
            std::cerr << "Usage: decrypt <in_parquet>" << std::endl;
            return -1;
        }
        std::string inFile = argv[2];

        std::cout << "[Client] Loading context and Secret keys..." << std::endl;
        // TODO: cryptoManager.LoadAllKeysFromDisk("keys/");

        std::cout << "[Client] Decrypting and verifying results from " << inFile << "..." << std::endl;
        // TODO: Read Parquet, decrypt AES and FHE columns, print to console as JSON
    } 
    
    else {
        std::cerr << "Unknown command: " << command << std::endl;
        PrintUsage(argv[0]);
        return -1;
    }

    return 0;
}