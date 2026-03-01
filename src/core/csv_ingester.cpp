#include "../../include/csv_ingester.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#undef duration
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/writer.h>

std::vector<std::string> SplitCSVLine(const std::string& line) {
    std::vector<std::string> result;
    std::stringstream ss(line);
    std::string cell;
    while (std::getline(ss, cell, ',')) {
        result.push_back(cell);
    }
    return result;
}

std::string DummyAESEncrypt(const std::string& plaintext) {
    return "[AES_LOCKED]_" + plaintext;
}

void IngestCSV(const std::string& csvFile, 
               const std::string& parquetOutFile, 
               const DatasetSchema& schema, 
               CryptoContextManager& cryptoManager) {
                   
    std::ifstream file(csvFile);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open CSV file " << csvFile << std::endl;
        return;
    }

    arrow::BinaryBuilder name_builder;      
    arrow::StringBuilder job_builder;      
    arrow::BinaryBuilder basepay_builder;  
    arrow::BinaryBuilder overtime_builder;

    std::string line;
    bool isHeader = true;
    int rowCount = 0;

    auto cc = cryptoManager.GetContext();
    auto pubKey = cryptoManager.GetPublicKey();

    std::cout << "[Ingester] Starting hybrid encryption pipeline..." << std::endl;

    // 2. Read the CSV row by row
    while (std::getline(file, line)) {
        if (isHeader) { isHeader = false; continue; } // Skip the header row

        auto cells = SplitCSVLine(line);
        if (cells.size() < 4) continue; // Safety check

        for (size_t i = 0; i < 4; ++i) {
            EncryptionType rule = schema.columns[i].encType;
            std::string raw_value = cells[i];

            // 3. THE ROUTING LOGIC
            if (rule == EncryptionType::PLAINTEXT) {
                (void)job_builder.Append(raw_value);
            } 
            else if (rule == EncryptionType::AES_GCM) {
                std::string aes_blob = cryptoManager.AESEncrypt(raw_value);
                (void)name_builder.Append(aes_blob);
            } 
            else if (rule == EncryptionType::FHE_BFV) {
                // Convert string to integer
                int64_t num_value = std::stoll(raw_value);
                
                // Pack, Encrypt, and Serialize
                auto plaintext = cc->MakePackedPlaintext({num_value});
                auto ciphertext = cc->Encrypt(pubKey, plaintext);
                std::string fhe_blob = cryptoManager.SerializeCiphertext(ciphertext);
                
                if (i == 2) (void)basepay_builder.Append(fhe_blob);
                if (i == 3) (void)overtime_builder.Append(fhe_blob);
            }
        }
        rowCount++;
        if (rowCount % 10 == 0) std::cout << "Processed " << rowCount << " rows..." << std::endl;
    }

    // 4. Finalize Arrow Arrays
    std::shared_ptr<arrow::Array> name_arr, job_arr, base_arr, over_arr;
    (void)name_builder.Finish(&name_arr);
    (void)job_builder.Finish(&job_arr);
    (void)basepay_builder.Finish(&base_arr);
    (void)overtime_builder.Finish(&over_arr);

    // 5. Build Schema & Table
    auto arrow_schema = arrow::schema({
        arrow::field("EmployeeName", arrow::binary()),
        arrow::field("JobTitle", arrow::utf8()),
        arrow::field("BasePay", arrow::binary()),
        arrow::field("OvertimePay", arrow::binary())
    });

    auto table = arrow::Table::Make(arrow_schema, {name_arr, job_arr, base_arr, over_arr});

    // 6. Write to Parquet
    auto maybe_outfile = arrow::io::FileOutputStream::Open(parquetOutFile);
    if (maybe_outfile.ok()) {
        std::shared_ptr<arrow::io::FileOutputStream> outfile = *maybe_outfile;
        (void)parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, 1);
        std::cout << "[Producer] Hybrid Parquet file created successfully! (" << rowCount << " rows)" << std::endl;
    }
}