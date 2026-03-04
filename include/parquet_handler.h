#ifndef PARQUET_HANDLER_H
#define PARQUET_HANDLER_H

#include <string>
#include <cstdint>
#include "crypto_manager.h"

// Declaration of the function so other files can see it
void WriteShieldedParquet(const std::string& filename, int64_t id, const std::string& encrypted_blob);
void DecryptShieldedParquet(const std::string& inFile, CryptoContextManager& cryptoManager);

std::string ReadShieldedParquet(const std::string& filename);
void ProcessCloudCompute(const std::string& inFile, const std::string& outFile, CryptoContextManager& cryptoManager);

#endif