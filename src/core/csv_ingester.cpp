#include "../../include/csv_ingester.h"
#include "../../include/fhe_engine.h"
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

        for (size_t i = 0; i < 4; ++i) {
            auto rule = schema.columns[i].encType;
            std::string val = cells[i];

            if (rule == EncryptionType::PLAINTEXT) {
                job_builder.Append(val);
            }
            else if (rule == EncryptionType::AES_GCM) {
                std::string enc = cryptoManager.AESEncrypt(val);

                //std::cout << "[INGEST] Plain: " << val << std::endl;
                //std::cout << "[INGEST] Encrypted(Base64): " << enc << std::endl;

                if (!name_builder.Append(enc).ok()) {
                    std::cerr << "Append failed\n";
                }
                //name_builder.Append(cryptoManager.AESEncrypt(val));
            }
            else if (rule == EncryptionType::FHE_BFV) {
                std::string enc = fhe->Encrypt(std::stoll(val));

                if (i == 2) basepay_builder.Append(enc);
                if (i == 3) overtime_builder.Append(enc);
            }
        }
    }

    std::shared_ptr<arrow::Array> name_arr, job_arr, base_arr, over_arr;
    name_builder.Finish(&name_arr);
    job_builder.Finish(&job_arr);
    basepay_builder.Finish(&base_arr);
    overtime_builder.Finish(&over_arr);

    auto schema_arrow = arrow::schema({
        arrow::field("EmployeeName", arrow::utf8()),
        arrow::field("JobTitle", arrow::utf8()),
        arrow::field("BasePay", arrow::utf8()),
        arrow::field("OvertimePay", arrow::utf8())
    });

    auto table = arrow::Table::Make(schema_arrow, {name_arr, job_arr, base_arr, over_arr});

    auto outfile = arrow::io::FileOutputStream::Open(parquetOutFile).ValueOrDie();
    parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, 1);
}