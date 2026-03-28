#include "../../include/fhe_engine.h"
#include <string>

class SimpleFHEEngine : public FHEEngine {
public:
    std::string Encrypt(int64_t value) override {
        return "ENC(" + std::to_string(value) + ")";
    }

    int64_t Decrypt(const std::string& cipher) override {
        return std::stoll(cipher.substr(4, cipher.size() - 5));
    }

    std::string Add(const std::string& a, const std::string& b) override {
        return Encrypt(Decrypt(a) + Decrypt(b));
    }

    void Initialize() override {}
    void GenerateKeys() override {}
    void SaveKeys(const std::string&) override {}
    void LoadEvalKeys(const std::string&) override {}
    void LoadAllKeys(const std::string&) override {}
};

// Factory
FHEEngine* CreateFHEEngine() {
    return new SimpleFHEEngine();
}