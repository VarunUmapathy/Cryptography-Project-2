#include "../../include/fhe_engine.h"
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <iostream>
#include <ctime>

/* =========================
   ENGINE
   ========================= */
class AdvancedFHEEngine : public FHEEngine {
private:
    const int MAX_NOISE = 100;

public:

    void Initialize() override {
        std::srand(time(nullptr));
    }

    void GenerateKeys() override {
        // Simulated keys
    }

    /* =========================
       ENCRYPT
       ========================= */
    std::string Encrypt(int64_t value) override {
        int noise = rand() % 5 + 1;

        Ciphertext ct;
        ct.value = value + noise;  // inject noise
        ct.noise = noise;

        return Serialize(ct);
    }

    /* =========================
       DECRYPT
       ========================= */
    int64_t Decrypt(const std::string& cipher) override {
        Ciphertext ct = Deserialize(cipher);

        if (ct.noise > MAX_NOISE) {
            throw std::runtime_error("Noise overflow - decryption failed");
        }

        return ct.value - ct.noise; // remove noise
    }

    /* =========================
       ADD
       ========================= */
    std::string Add(const std::string& a, const std::string& b) override {
        Ciphertext A = Deserialize(a);
        Ciphertext B = Deserialize(b);

        Ciphertext R;
        R.value = A.value + B.value;
        R.noise = A.noise + B.noise + 1;
        if (R.noise > 50) {
            std::cout << "[FHE] Auto-bootstrapping triggered\n";
            R = Deserialize(Bootstrap(Serialize(R)));
        }
        return Serialize(R);
    }

    /* =========================
       MULTIPLY (NEW)
       ========================= */
    std::string Multiply(const std::string& a, const std::string& b) {
        Ciphertext A = Deserialize(a);
        Ciphertext B = Deserialize(b);

        Ciphertext R;
        R.value = A.value * B.value;
        R.noise = A.noise * B.noise + 5;
        if (R.noise > 50) {
            std::cout << "[FHE] Auto-bootstrapping triggered\n";
            R = Deserialize(Bootstrap(Serialize(R)));
        }
        return Serialize(R);
    }

    /* =========================
       BOOTSTRAP (NEW)
       ========================= */
    std::string Bootstrap(const std::string& a) {
        Ciphertext A = Deserialize(a);

        Ciphertext R;
        R.value = A.value;  // keep encoded value
        R.noise = 1;        // reset noise

        return Serialize(R);
    }

    /* =========================
   SERIALIZATION
   ========================= */
    std::string Serialize(const Ciphertext& ct) {
        return std::to_string(ct.value) + "|" + std::to_string(ct.noise);
    }

    Ciphertext Deserialize(const std::string& s) {
        size_t pos = s.find("|");
        if (pos == std::string::npos)
            throw std::runtime_error("Invalid ciphertext format");

        return {
            std::stoll(s.substr(0, pos)),
            std::stoi(s.substr(pos + 1))
        };
    }

    /* =========================
       KEY OPS
       ========================= */
    void SaveKeys(const std::string&) override {}
    void LoadEvalKeys(const std::string&) override {}
    void LoadAllKeys(const std::string&) override {}
};

/* =========================
   FACTORY
   ========================= */
FHEEngine* CreateFHEEngine() {
    return new AdvancedFHEEngine();
}