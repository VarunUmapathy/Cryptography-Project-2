#include "../../include/parquet_handler.h"
#include "../../include/crypto_manager.h"
#include "../../include/fhe_engine.h"

#undef duration
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/writer.h>
#include <parquet/arrow/reader.h>

#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/* =========================
   WRITE SINGLE ROW (OPTIONAL)
   ========================= */
void WriteShieldedParquet(const std::string& filename, int64_t id, const std::string& encrypted_blob) {
    auto schema = arrow::schema({
        arrow::field("ID", arrow::int64()),
        arrow::field("Encrypted_Salary", arrow::binary())
    });

    arrow::Int64Builder id_builder;
    arrow::BinaryBuilder salary_builder;

    (void)id_builder.Append(id);
    (void)salary_builder.Append(encrypted_blob);

    std::shared_ptr<arrow::Array> id_array, salary_array;
    (void)id_builder.Finish(&id_array);
    (void)salary_builder.Finish(&salary_array);

    auto table = arrow::Table::Make(schema, {id_array, salary_array});

    auto outfile_res = arrow::io::FileOutputStream::Open(filename);
    if (!outfile_res.ok()) {
        std::cerr << "Error opening file: " << outfile_res.status().ToString() << std::endl;
        return;
    }

    auto outfile = *outfile_res;
    auto status = parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, 1);

    if (status.ok())
        std::cout << "[Producer] File written: " << filename << std::endl;
    else
        std::cerr << "Parquet Write Error: " << status.ToString() << std::endl;
}

/* =========================
   DECRYPT FUNCTION
   ========================= */
void DecryptShieldedParquet(
    const std::string& inFile,
    CryptoContextManager& cryptoManager,
    FHEEngine* fhe
) {
    // Open file
    auto open_res = arrow::io::ReadableFile::Open(inFile);
    if (!open_res.ok()) {
        std::cerr << "Error opening file\n";
        return;
    }

    auto infile = *open_res;

    // Build reader
    std::unique_ptr<parquet::arrow::FileReader> reader;
    parquet::arrow::FileReaderBuilder builder;
    (void)builder.Open(infile);
    (void)builder.Build(&reader);

    // Read table
    std::shared_ptr<arrow::Table> table;
    (void)reader->ReadTable(&table);

    // Extract columns safely
    auto name_col = std::static_pointer_cast<arrow::StringArray>(
        table->GetColumnByName("EmployeeName")->chunk(0));

    auto job_col = std::static_pointer_cast<arrow::StringArray>(
        table->GetColumnByName("JobTitle")->chunk(0));

    auto basepay_col = std::static_pointer_cast<arrow::StringArray>(
        table->GetColumnByName("BasePay")->chunk(0));

    auto overtime_col = std::static_pointer_cast<arrow::StringArray>(
        table->GetColumnByName("OvertimePay")->chunk(0));

    json results = json::array();

    for (int64_t i = 0; i < table->num_rows(); i++) {
        json row;

        std::string name_blob = name_col->GetString(i);
        std::string basepay_blob = basepay_col->GetString(i);
        std::string overtime_blob = overtime_col->GetString(i);

        // AES decrypt
        //std::cout << "[READ] From Parquet: " << name_blob << std::endl;
        row["EmployeeName"] = cryptoManager.AESDecrypt(name_blob);

        // Plaintext
        row["JobTitle"] = job_col->GetString(i);

        // FHE decrypt via abstraction
        row["BasePay_Doubled"] = fhe->Decrypt(basepay_blob);
        row["OvertimePay"] = fhe->Decrypt(overtime_blob);

        results.push_back(row);
    }

    std::cout << "\n--- FINAL DECRYPTED DATA ---\n";
    std::cout << results.dump(4) << std::endl;
}

/* =========================
   CLOUD COMPUTE
   ========================= */
void ProcessCloudCompute(
    const std::string& inFile,
    const std::string& outFile,
    CryptoContextManager& cryptoManager,
    FHEEngine* fhe
) {
    // Open file
    auto open_res = arrow::io::ReadableFile::Open(inFile);
    if (!open_res.ok()) {
        std::cerr << "Error opening input file\n";
        return;
    }

    auto infile = *open_res;

    // Build reader
    std::unique_ptr<parquet::arrow::FileReader> reader;
    parquet::arrow::FileReaderBuilder builder;
    (void)builder.Open(infile);
    (void)builder.Build(&reader);

    // Read table
    std::shared_ptr<arrow::Table> table;
    (void)reader->ReadTable(&table);

    // Extract columns
    auto name_col = std::static_pointer_cast<arrow::StringArray>(
        table->GetColumnByName("EmployeeName")->chunk(0));

    auto job_col = std::static_pointer_cast<arrow::StringArray>(
        table->GetColumnByName("JobTitle")->chunk(0));

    auto basepay_col = std::static_pointer_cast<arrow::StringArray>(
        table->GetColumnByName("BasePay")->chunk(0));

    auto overtime_col = std::static_pointer_cast<arrow::StringArray>(
        table->GetColumnByName("OvertimePay")->chunk(0));

    // Builders for new file
    arrow::StringBuilder new_name, new_basepay, new_overtime;
    arrow::StringBuilder new_job;
    //std::cout << "[DEBUG] Input rows: " << table->num_rows() << "\n";
    for (int64_t i = 0; i < table->num_rows(); i++) {
        std::string enc_base = basepay_col->GetString(i);

        // Pass-through columns
        (void)new_name.Append(name_col->GetString(i));
        (void)new_job.Append(job_col->GetString(i));
        (void)new_overtime.Append(overtime_col->GetString(i));

        // Homomorphic (simulated) addition
        std::string doubled = fhe->Add(enc_base, enc_base);
        (void)new_basepay.Append(doubled);
    }

    // Finalize arrays
    std::shared_ptr<arrow::Array> name_arr, job_arr, base_arr, over_arr;
    (void)new_name.Finish(&name_arr);
    (void)new_job.Finish(&job_arr);
    (void)new_basepay.Finish(&base_arr);
    (void)new_overtime.Finish(&over_arr);

    auto new_table = arrow::Table::Make(
        table->schema(),
        {name_arr, job_arr, base_arr, over_arr}
    );

    auto outfile_res = arrow::io::FileOutputStream::Open(outFile);
    if (!outfile_res.ok()) {
        std::cerr << "Error opening output file\n";
        return;
    }

    auto outfile = *outfile_res;

    (void)parquet::arrow::WriteTable(
        *new_table,
        arrow::default_memory_pool(),
        outfile,
        1
    );

    std::cout << "[Cloud] Computation complete: " << outFile << std::endl;
}