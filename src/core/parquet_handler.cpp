#include "../../include/parquet_handler.h"
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