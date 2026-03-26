#ifndef CSV_INGESTER_H
#define CSV_INGESTER_H

#include <string>
#include "schema_parser.h"
#include "crypto_manager.h"

class FHEEngine;  // forward declaration

void IngestCSV(const std::string& csvFile,
               const std::string& parquetOutFile,
               const DatasetSchema& schema,
               CryptoContextManager& cryptoManager,
               FHEEngine* fhe);
               
#endif // CSV_INGESTER_H