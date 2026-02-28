#ifndef PARQUET_HANDLER_H
#define PARQUET_HANDLER_H

#include <string>
#include <cstdint>

// Declaration of the function so other files can see it
void WriteShieldedParquet(const std::string& filename, int64_t id, const std::string& encrypted_blob);

#endif