#pragma once
#include <string>
#include <vector>

struct Ciphertext {
    std::vector<int64_t> c0, c1, c2;   
    int noise;                 
};

struct PublicKey {
    std::vector<int64_t> a;
    std::vector<int64_t> b;
};

struct SecretKey {
    std::vector<int64_t> s;
};

struct EvalKey {
    std::vector<int64_t> rlk0; // NEW
    std::vector<int64_t> rlk1; // NEW
    std::vector<int64_t> relin_key;
};

class FHEEngine {
public:
    virtual void Initialize() = 0;
    virtual void GenerateKeys() = 0;

    virtual std::string Serialize(const Ciphertext& ct) = 0;
    virtual Ciphertext Deserialize(const std::string& s) = 0;

    // SIMD-aware (still compatible)
    virtual std::string Encrypt(int64_t value) = 0;
    virtual int64_t Decrypt(const std::string& ciphertext) = 0;

    // NEW (optional SIMD support)
    virtual std::string EncryptBatch(const std::vector<int64_t>& values) = 0;
    virtual std::vector<int64_t> DecryptBatch(const std::string& ciphertext) = 0;

    virtual std::string Add(const std::string& a, const std::string& b) = 0;
    virtual std::string Multiply(const std::string& a, const std::string& b) = 0;
    virtual std::string Bootstrap(const std::string& a) = 0;

    virtual void SaveKeys(const std::string& path) = 0;
    virtual void LoadEvalKeys(const std::string& path) = 0;
    virtual void LoadAllKeys(const std::string& path) = 0;

    virtual ~FHEEngine() = default;
};

FHEEngine* CreateFHEEngine();