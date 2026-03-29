#include "../../include/parquet_handler.h"
#include "../../include/crypto_manager.h"
#include "../../include/fhe_engine.h"
#include "../../include/timer.h"

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

    if (!id_builder.Append(id).ok()) {
        std::cerr << "Append failed\n";
        return;
    }
    if (!salary_builder.Append(encrypted_blob).ok()) {
        std::cerr << "Append failed\n";
        return;
    }
    std::shared_ptr<arrow::Array> id_array, salary_array;
    if (!id_builder.Finish(&id_array).ok()) {
        std::cerr << "Finish failed\n";
        return;
    }
    if (!salary_builder.Finish(&salary_array).ok()) {
        std::cerr << "Finish failed\n";
        return;
    }
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
    Timer t;
    double aes_dec_time = 0;
    double fhe_dec_time = 0;
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
    int rows = table->num_rows();
    for (int64_t i = 0; i < table->num_rows(); i++) {
        json row;

        std::string name_blob = name_col->GetString(i);
        std::string basepay_blob = basepay_col->GetString(i);
        std::string overtime_blob = overtime_col->GetString(i);

        // AES decrypt
        //std::cout << "[READ] From Parquet: " << name_blob << std::endl
        t.Start();
        row["EmployeeName"] = cryptoManager.AESDecrypt(name_blob);
        aes_dec_time += t.Stop();

        // Plaintext
        row["JobTitle"] = job_col->GetString(i);

        // FHE decrypt via abstraction
        t.Start();
        row["BasePay_Doubled"] = fhe->Decrypt(basepay_blob);
        fhe_dec_time += t.Stop();
        row["OvertimePay"] = fhe->Decrypt(overtime_blob);

        results.push_back(row);
    }
    std::cout << "\n--- DECRYPT METRICS ---\n";
    std::cout << "Rows: " << rows << "\n";
    std::cout << "AES Decrypt Time: " << aes_dec_time << " sec\n";
    std::cout << "FHE Decrypt Time: " << fhe_dec_time << " sec\n";
    std::cout << "Throughput: " << rows / (aes_dec_time + fhe_dec_time) << " rows/sec\n";
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
    Timer t;
    double compute_time = 0;
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
    int rows = table->num_rows();
    //std::cout << "[DEBUG] Input rows: " << table->num_rows() << "\n";
    for (int64_t i = 0; i < table->num_rows(); i++) {
        std::string enc_base = basepay_col->GetString(i);

        // Pass-through columns
        if (!new_name.Append(name_col->GetString(i)).ok()) {
            std::cerr << "Append name failed\n"; return;
        }

        if (!new_job.Append(job_col->GetString(i)).ok()) {
            std::cerr << "Append job failed\n"; return;
        }

        if (!new_overtime.Append(overtime_col->GetString(i)).ok()) {
            std::cerr << "Append overtime failed\n"; return;
        }

        // Homomorphic (simulated) addition
        t.Start();
        std::string doubled = fhe->Add(enc_base, enc_base);
        compute_time += t.Stop();
        if (!new_basepay.Append(doubled).ok()) {
            std::cerr << "Append basepay failed\n"; return;
        }
    }
    std::cout << "\n--- CLOUD METRICS ---\n";
    std::cout << "Rows: " << rows << "\n";
    std::cout << "Compute Time: " << compute_time << " sec\n";
    std::cout << "Throughput: " << rows / compute_time << " rows/sec\n";
    // Finalize arrays
    std::shared_ptr<arrow::Array> name_arr, job_arr, base_arr, over_arr;
    if (!new_name.Finish(&name_arr).ok() ||
        !new_job.Finish(&job_arr).ok() ||
        !new_basepay.Finish(&base_arr).ok() ||
        !new_overtime.Finish(&over_arr).ok()) {
        std::cerr << "Finish failed\n";
        return;
    }

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

    auto write_status = parquet::arrow::WriteTable(
        *new_table,
        arrow::default_memory_pool(),
        outfile,
        1
    );

    if (!write_status.ok()) {
        std::cerr << "Parquet write failed: "
                << write_status.ToString() << std::endl;
        return;
    }

    std::cout << "[Cloud] Computation complete: " << outFile << std::endl;
}