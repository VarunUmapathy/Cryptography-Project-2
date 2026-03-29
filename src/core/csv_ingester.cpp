#include "../../include/csv_ingester.h"
#include "../../include/fhe_engine.h"
#include "../../include/timer.h"
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
               CryptoContextManager& cryptoManager,
               FHEEngine* fhe) {
    double aes_time = 0;
    double fhe_time = 0;
    int rows = 0;

    Timer t;
    std::ifstream file(csvFile);
    if (!file.is_open()) return;

    arrow::StringBuilder name_builder;
    arrow::StringBuilder job_builder;
    arrow::StringBuilder basepay_builder;
    arrow::StringBuilder overtime_builder;

    std::string line;
    bool isHeader = true;

    while (std::getline(file, line)) {
        if (isHeader) { isHeader = false; continue; }

        auto cells = SplitCSVLine(line);
        rows++;
        for (size_t i = 0; i < 4; ++i) {
            auto rule = schema.columns[i].encType;
            std::string val = cells[i];
            if (rule == EncryptionType::PLAINTEXT) {
                if (!job_builder.Append(val).ok()) {
                    std::cerr << "Append failed\n";
                }
            }
            else if (rule == EncryptionType::AES_GCM) {
                t.Start();
                std::string enc = cryptoManager.AESEncrypt(val);

                //std::cout << "[INGEST] Plain: " << val << std::endl;
                //std::cout << "[INGEST] Encrypted(Base64): " << enc << std::endl;
                aes_time += t.Stop();
                if (!name_builder.Append(enc).ok()) {
                    std::cerr << "Append failed\n";
                }
                //name_builder.Append(cryptoManager.AESEncrypt(val));
            }
            else if (rule == EncryptionType::FHE_BFV) {
                t.Start();
                std::string enc = fhe->Encrypt(std::stoll(val));
                fhe_time += t.Stop();
                if (i == 2){
                    if (!basepay_builder.Append(enc).ok()) {
                        std::cerr << "Append failed\n";
                    }
                }
                if (i == 3){
                    if (!overtime_builder.Append(enc).ok()) {
                        std::cerr << "Append failed\n";
                    }
                }
            }
        }
    }
    std::cout << "\n--- INGEST METRICS ---\n";
    std::cout << "Rows: " << rows << "\n";
    std::cout << "AES Time: " << aes_time << " sec\n";
    std::cout << "FHE Time: " << fhe_time << " sec\n";
    std::cout << "Throughput: " << rows / (aes_time + fhe_time) << " rows/sec\n";
    std::shared_ptr<arrow::Array> name_arr, job_arr, base_arr, over_arr;
    if (!name_builder.Finish(&name_arr).ok()) {
        std::cerr << "name finish failed\n";
    }
    if (!job_builder.Finish(&job_arr).ok()) {
        std::cerr << "name finish failed\n";
    }
    if (!basepay_builder.Finish(&base_arr).ok()) {
        std::cerr << "name finish failed\n";
    }
    if (!overtime_builder.Finish(&over_arr).ok()) {
        std::cerr << "name finish failed\n";
    }

    auto schema_arrow = arrow::schema({
        arrow::field("EmployeeName", arrow::utf8()),
        arrow::field("JobTitle", arrow::utf8()),
        arrow::field("BasePay", arrow::utf8()),
        arrow::field("OvertimePay", arrow::utf8())
    });

    auto table = arrow::Table::Make(schema_arrow, {name_arr, job_arr, base_arr, over_arr});

    auto outfile = arrow::io::FileOutputStream::Open(parquetOutFile).ValueOrDie();
    auto status = parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, 1);
    if (!status.ok()) {
        std::cerr << "WriteTable failed: " << status.ToString() << "\n";
    }
}