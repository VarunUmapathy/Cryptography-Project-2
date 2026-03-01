#ifndef CSV_INGESTER_H
#define CSV_INGESTER_H

#include <string>
#include "schema_parser.h"
#include "crypto_manager.h"

// Reads the CSV, encrypts the data based on the schema, and writes the Parquet file
void IngestCSV(const std::string& csvFile, 
               const std::string& parquetOutFile, 
               const DatasetSchema& schema, 
               CryptoContextManager& cryptoManager);

#endif // CSV_INGESTER_H