#include "../../include/parquet_handler.h"
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/writer.h>
#include <iostream>

void WriteShieldedParquet(const std::string& filename, int64_t id, const std::string& encrypted_blob) {
    // 1. Setup Schema
    auto schema = arrow::schema({
        arrow::field("ID", arrow::int64()),
        arrow::field("Encrypted_Salary", arrow::binary())
    });

    // 2. Build Data
    arrow::Int64Builder id_builder;
    arrow::BinaryBuilder salary_builder;

    // We use .ValueOrDie() or check status to ensure builders are initialized
    if (!id_builder.Append(id).ok()) return;
    if (!salary_builder.Append(encrypted_blob).ok()) return;

    std::shared_ptr<arrow::Array> id_array;
    std::shared_ptr<arrow::Array> salary_array;
    if (!id_builder.Finish(&id_array).ok()) return;
    if (!salary_builder.Finish(&salary_array).ok()) return;

    auto table = arrow::Table::Make(schema, {id_array, salary_array});

    // 3. SECURE FILE OPENING (This is likely where the Segfault lived)
    auto maybe_outfile = arrow::io::FileOutputStream::Open(filename);
    if (!maybe_outfile.ok()) {
        std::cerr << "Error: Could not open file for writing: " << maybe_outfile.status().ToString() << std::endl;
        return;
    }
    std::shared_ptr<arrow::io::FileOutputStream> outfile = *maybe_outfile;

    // 4. Write Table
    auto status = parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, 1);
    
    if (status.ok()) {
        std::cout << "[Producer] Shielded Parquet file created: " << filename << std::endl;
    } else {
        std::cerr << "Parquet Write Error: " << status.ToString() << std::endl;
    }
}