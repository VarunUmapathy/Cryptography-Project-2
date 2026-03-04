#include "../../include/parquet_handler.h"
#include "../../include/crypto_manager.h"
#undef duration
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/writer.h>
#include <parquet/arrow/reader.h>
#include <iostream>

void WriteShieldedParquet(const std::string& filename, int64_t id, const std::string& encrypted_blob) {
    auto schema = arrow::schema({
        arrow::field("ID", arrow::int64()),
        arrow::field("Encrypted_Salary", arrow::binary())
    });

    arrow::Int64Builder id_builder;
    arrow::BinaryBuilder salary_builder;

    if (!id_builder.Append(id).ok()) return;
    if (!salary_builder.Append(encrypted_blob).ok()) return;

    std::shared_ptr<arrow::Array> id_array;
    std::shared_ptr<arrow::Array> salary_array;
    if (!id_builder.Finish(&id_array).ok()) return;
    if (!salary_builder.Finish(&salary_array).ok()) return;

    auto table = arrow::Table::Make(schema, {id_array, salary_array});

    auto maybe_outfile = arrow::io::FileOutputStream::Open(filename);
    if (!maybe_outfile.ok()) {
        std::cerr << "Error: Could not open file for writing: " << maybe_outfile.status().ToString() << std::endl;
        return;
    }
    std::shared_ptr<arrow::io::FileOutputStream> outfile = *maybe_outfile;

    auto status = parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, 1);
    
    if (status.ok()) {
        std::cout << "[Producer] Shielded Parquet file created: " << filename << std::endl;
    } else {
        std::cerr << "Parquet Write Error: " << status.ToString() << std::endl;
    }
}

std::string ReadShieldedParquet(const std::string& filename) {
    // 1. Open the file
    auto open_res = arrow::io::ReadableFile::Open(filename);
    if (!open_res.ok()) return "";
    std::shared_ptr<arrow::io::ReadableFile> infile = *open_res;

    // 2. Create Parquet Reader using the safe Builder pattern
    std::unique_ptr<parquet::arrow::FileReader> reader;
    parquet::arrow::FileReaderBuilder builder;
    if (!builder.Open(infile).ok()) {
        std::cerr << "Error: Could not open Parquet file stream." << std::endl;
        return "";
    }
    if (!builder.Build(&reader).ok()) {
        std::cerr << "Error: Could not build Parquet reader." << std::endl;
        return "";
    }

    // 3. Read the whole table back into memory
    std::shared_ptr<arrow::Table> table;
    auto status = reader->ReadTable(&table);
    if (!status.ok()) return "";

    // 4. Extract the "Encrypted_Salary" column 
    auto column = table->GetColumnByName("Encrypted_Salary");
    if (!column) {
        std::cerr << "Error: Column 'Encrypted_Salary' not found in Parquet file." << std::endl;
        return "";
    }
    
    // In modern Arrow, GetColumnByName returns a ChunkedArray directly.
    auto binary_array = std::static_pointer_cast<arrow::BinaryArray>(column->chunk(0));

    // 5. Get the raw bytes from the first row
    return binary_array->GetString(0);
}

void ProcessCloudCompute(const std::string& inFile, const std::string& outFile, CryptoContextManager& cryptoManager) {
    // 1. Open the original file
    auto open_res = arrow::io::ReadableFile::Open(inFile);
    if (!open_res.ok()) return;
    std::shared_ptr<arrow::io::ReadableFile> infile_stream = *open_res;

    std::unique_ptr<parquet::arrow::FileReader> reader;
    parquet::arrow::FileReaderBuilder builder;
    
    // Explicitly cast to (void) to silence compiler warnings
    (void)builder.Open(infile_stream);
    (void)builder.Build(&reader);

    std::shared_ptr<arrow::Table> table;
    (void)reader->ReadTable(&table);

    // 2. Extract our columns
    auto name_col = std::static_pointer_cast<arrow::BinaryArray>(table->GetColumnByName("EmployeeName")->chunk(0));
    auto job_col = std::static_pointer_cast<arrow::StringArray>(table->GetColumnByName("JobTitle")->chunk(0));
    auto basepay_col = std::static_pointer_cast<arrow::BinaryArray>(table->GetColumnByName("BasePay")->chunk(0));
    auto overtime_col = std::static_pointer_cast<arrow::BinaryArray>(table->GetColumnByName("OvertimePay")->chunk(0));

    // 3. Setup builders for the NEW file
    arrow::BinaryBuilder new_name, new_basepay, new_overtime;
    arrow::StringBuilder new_job;
    auto cc = cryptoManager.GetContext();

    for (int64_t i = 0; i < table->num_rows(); i++) {
        // Pass Plaintext and AES strings straight through unchanged
        (void)new_name.Append(name_col->GetString(i));
        (void)new_job.Append(job_col->GetString(i));
        (void)new_overtime.Append(overtime_col->GetString(i));

        // THE BLIND MATH: Double the BasePay
        std::string encrypted_basepay = basepay_col->GetString(i);
        auto ciphertext = cryptoManager.DeserializeCiphertext(encrypted_basepay);
        
        // FHE Addition (Salary + Salary)
        auto doubled_ciphertext = cc->EvalAdd(ciphertext, ciphertext); 
        
        std::string updated_blob = cryptoManager.SerializeCiphertext(doubled_ciphertext);
        (void)new_basepay.Append(updated_blob);
    }

    // 4. Write the computed Parquet file back to disk
    std::shared_ptr<arrow::Array> name_arr, job_arr, base_arr, over_arr;
    (void)new_name.Finish(&name_arr); (void)new_job.Finish(&job_arr);
    (void)new_basepay.Finish(&base_arr); (void)new_overtime.Finish(&over_arr);

    auto new_table = arrow::Table::Make(table->schema(), {name_arr, job_arr, base_arr, over_arr});
    
    auto maybe_outfile = arrow::io::FileOutputStream::Open(outFile);
    std::shared_ptr<arrow::io::FileOutputStream> outfile_stream = *maybe_outfile;
    (void)parquet::arrow::WriteTable(*new_table, arrow::default_memory_pool(), outfile_stream, 1);
    
    std::cout << "[Cloud] Successfully computed and wrote new Parquet file: " << outFile << std::endl;
}