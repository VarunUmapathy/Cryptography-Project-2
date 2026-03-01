#include "../include/crypto_manager.h"
#include "../include/schema_parser.h"
#include "../include/csv_ingester.h"
#include <iostream>

using namespace lbcrypto;

int main() {
    std::cout << "=== Shielded-Parquet Hybrid Engine ===" << std::endl;

    // 1. Initialize the cryptographic keys (FHE + AES)
    CryptoContextManager cryptoManager;
    cryptoManager.InitializeBFVContext();
    cryptoManager.GenerateKeys();
    cryptoManager.GenerateAESKey();

    // 2. Load the JSON Schema rules
    std::cout << "\n[Client] Loading encryption schema..." << std::endl;
    DatasetSchema schema = ParseSchema("../src/schema.json");
    
    if (schema.columns.empty()) {
        std::cerr << "Error: Failed to load schema.json or schema is empty!" << std::endl;
        return -1;
    }

    // 3. Ingest the CSV, apply routing rules, and write Parquet
    std::cout << "[Client] Starting CSV ingestion and hybrid encryption..." << std::endl;
    IngestCSV("../src/data.csv", "sf_salaries_hybrid.parquet", schema, cryptoManager);

    return 0;
}