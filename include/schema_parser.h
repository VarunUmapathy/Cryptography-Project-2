#ifndef SCHEMA_PARSER_H
#define SCHEMA_PARSER_H

#include <string>
#include <vector>

// 1. Define the possible routes
enum class EncryptionType {
    PLAINTEXT,
    AES_GCM,
    FHE_BFV,
    UNKNOWN
};

// 2. Define what a single column looks like
struct ColumnConfig {
    std::string name;
    std::string dataType;
    EncryptionType encType;
};

// 3. Define the overall dataset schema
struct DatasetSchema {
    std::string datasetName;
    std::vector<ColumnConfig> columns;
};

// 4. Function to parse the JSON file from disk
DatasetSchema ParseSchema(const std::string& filepath);

#endif