#pragma once
#include <string>
#include <vector>

class FHEEngine {
public:
    virtual void Initialize() = 0;
    virtual void GenerateKeys() = 0;

    virtual std::string Encrypt(int64_t value) = 0;
    virtual std::string Add(const std::string& a, const std::string& b) = 0;

    virtual int64_t Decrypt(const std::string& ciphertext) = 0;

    virtual void SaveKeys(const std::string& path) = 0;
    virtual void LoadEvalKeys(const std::string& path) = 0;
    virtual void LoadAllKeys(const std::string& path) = 0;

    virtual ~FHEEngine() = default;
};