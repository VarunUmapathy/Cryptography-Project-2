#include "../../include/schema_parser.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

// Helper function to map string to Enum
EncryptionType StringToEnum(const std::string& typeStr) {
    if (typeStr == "PLAINTEXT") return EncryptionType::PLAINTEXT;
    if (typeStr == "AES_GCM") return EncryptionType::AES_GCM;
    if (typeStr == "FHE_BFV") return EncryptionType::FHE_BFV;
    return EncryptionType::UNKNOWN;
}

DatasetSchema ParseSchema(const std::string& filepath) {
    DatasetSchema schema;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open schema file: " << filepath << std::endl;
        return schema;
    }

    // Parse the JSON file
    json j;
    file >> j;

    schema.datasetName = j["dataset_name"];

    // Loop through the JSON array and build our C++ vector
    for (const auto& col : j["columns"]) {
        ColumnConfig config;
        config.name = col["name"];
        config.dataType = col["type"];
        config.encType = StringToEnum(col["encryption"]);
        
        schema.columns.push_back(config);
    }

    std::cout << "[Config] Successfully loaded schema for: " << schema.datasetName << std::endl;
    return schema;
}