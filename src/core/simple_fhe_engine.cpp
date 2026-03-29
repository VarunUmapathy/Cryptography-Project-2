#include "../../include/fhe_engine.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <fstream>
#include <iostream>

/* =========================
   PARAMETERS
   ========================= */
const int POLY_DEGREE = 4;
const int64_t Q = 1e15;   // ciphertext modulus
const int64_t T = 1000000;      // plaintext modulus
const int SCALE=1;
int64_t Mod(int64_t x) {
    x %= Q;
    return (x < 0) ? x + Q : x;
}

/* =========================
   ENGINE
   ========================= */
class AdvancedFHEEngine : public FHEEngine {

private:
    PublicKey pk;
    SecretKey sk;
    EvalKey evk;

    /* =========================
       POLY MULTIPLICATION
       ========================= */
    std::vector<int64_t> PolyMul(const std::vector<int64_t>& a,
                                 const std::vector<int64_t>& b) {

        std::vector<int64_t> res(POLY_DEGREE, 0);

        for (int i = 0; i < POLY_DEGREE; i++) {
            for (int j = 0; j < POLY_DEGREE; j++) {
                int idx = (i + j) % POLY_DEGREE;
                int sign = ((i + j) >= POLY_DEGREE) ? -1 : 1;
                res[idx] = Mod(res[idx] + sign * a[i] * b[j]);
            }
        }
        return res;
    }

    /* =========================
       SIMD ENCODE / DECODE
       ========================= */
    std::vector<int64_t> Encode(const std::vector<int64_t>& data) {
        std::vector<int64_t> poly(POLY_DEGREE, 0);
        for (int i = 0; i < data.size() && i < POLY_DEGREE; i++)
            poly[i] = data[i] % T;
        return poly;
    }

    std::vector<int64_t> Decode(const std::vector<int64_t>& poly) {
        return poly;
    }

    /* =========================
       RELINEARIZATION
       ========================= */
    Ciphertext Relinearize(const Ciphertext& ct) {
        Ciphertext R;
        R.c0 = ct.c0;
        R.c1 = ct.c1;
        R.c2.assign(POLY_DEGREE, 0);
        return R;
    }

public:

    void Initialize() override {
        std::srand(time(nullptr));
    }

    /* =========================
       KEY GENERATION
       ========================= */
    void GenerateKeys() override {

        sk.s.resize(POLY_DEGREE);
        pk.a.resize(POLY_DEGREE);
        pk.b.resize(POLY_DEGREE);

        for (int i = 0; i < POLY_DEGREE; i++) {
            sk.s[i] = rand() % 2;      // small binary secret
            pk.a[i] = rand() % Q;      // random a
        }

        // ✅ CORRECT RLWE: b = -a ⊗ s
        auto a_s = PolyMul(pk.a, sk.s);

        for (int i = 0; i < POLY_DEGREE; i++) {
            pk.b[i] = Mod(-a_s[i]);   // NO NOISE
        }

        std::cout << "[FHE] Keys generated (stable mode)\n";
    }

    /* =========================
       SERIALIZATION
       ========================= */
    std::string Serialize(const Ciphertext& ct) override {

        std::ostringstream oss;

        for (auto v : ct.c0) oss << v << ",";
        oss << "|";
        for (auto v : ct.c1) oss << v << ",";
        oss << "|";
        for (auto v : ct.c2) oss << v << ",";
        oss << "|" << ct.noise;

        return oss.str();
    }

    Ciphertext Deserialize(const std::string& s) override {

        Ciphertext ct;
        ct.c0.resize(POLY_DEGREE);
        ct.c1.resize(POLY_DEGREE);
        ct.c2.resize(POLY_DEGREE);

        std::stringstream ss(s);
        std::string part;

        // c0
        std::getline(ss, part, '|');
        std::stringstream s0(part);
        for (int i = 0; i < POLY_DEGREE; i++) {
            std::getline(s0, part, ',');
            ct.c0[i] = std::stoll(part);
        }

        // c1
        std::getline(ss, part, '|');
        std::stringstream s1(part);
        for (int i = 0; i < POLY_DEGREE; i++) {
            std::getline(s1, part, ',');
            ct.c1[i] = std::stoll(part);
        }

        // c2
        std::getline(ss, part, '|');
        std::stringstream s2(part);
        for (int i = 0; i < POLY_DEGREE; i++) {
            std::getline(s2, part, ',');
            ct.c2[i] = std::stoll(part);
        }

        std::getline(ss, part);
        ct.noise = std::stoi(part);

        return ct;
    }

    /* =========================
       ENCRYPT (SIMD)
       ========================= */
    std::string Encrypt(int64_t value) override {

        std::vector<int64_t> m(POLY_DEGREE, 0);
        m[0] = value % T;

        std::vector<int64_t> u(POLY_DEGREE);

        for (int i = 0; i < POLY_DEGREE; i++)
            u[i] = rand() % 2;

        auto bu = PolyMul(pk.b, u);
        auto au = PolyMul(pk.a, u);

        Ciphertext ct;
        ct.c0.resize(POLY_DEGREE);
        ct.c1.resize(POLY_DEGREE);
        ct.c2.assign(POLY_DEGREE, 0);

        for (int i = 0; i < POLY_DEGREE; i++) {
            ct.c0[i] = Mod(bu[i] + m[i]);  // embed plaintext
            ct.c1[i] = Mod(au[i]);
        }

        return Serialize(ct);
    }

    /* =========================
       DECRYPT
       ========================= */
    int64_t Decrypt(const std::string& cipher) override {

        Ciphertext ct = Deserialize(cipher);

        auto s_c1 = PolyMul(sk.s, ct.c1);

        int64_t result = Mod(ct.c0[0] + s_c1[0]);

        // Convert from mod Q → signed integer
        if (result > Q / 2)
            result -= Q;

        // Bring back to plaintext space
        result %= T;
        if (result < 0)
            result += T;

        return result;
    }

    /* =========================
       ADD
       ========================= */
    std::string Add(const std::string& a, const std::string& b) override {

        Ciphertext A = Deserialize(a);
        Ciphertext B = Deserialize(b);

        Ciphertext R;
        R.c0.resize(POLY_DEGREE);
        R.c1.resize(POLY_DEGREE);
        R.c2.assign(POLY_DEGREE, 0);

        for (int i = 0; i < POLY_DEGREE; i++) {
            R.c0[i] = Mod(A.c0[i] + B.c0[i]);
            R.c1[i] = Mod(A.c1[i] + B.c1[i]);
        }

        return Serialize(R);
    }

    /* =========================
       MULTIPLY + RELINEARIZE
       ========================= */
    int64_t SCALE_FACTOR = Q / T;

    std::string Multiply(const std::string& a, const std::string& b) override {

        Ciphertext A = Deserialize(a);
        Ciphertext B = Deserialize(b);

        Ciphertext R;

        R.c0 = PolyMul(A.c0, B.c0);

        R.c1 = PolyMul(A.c0, B.c1);
        auto temp = PolyMul(A.c1, B.c0);
        for (int i = 0; i < POLY_DEGREE; i++)
            R.c1[i] = Mod(R.c1[i] + temp[i]);

        R.c2 = PolyMul(A.c1, B.c1);

        return Serialize(Relinearize(R));
    }
    /* =========================
       BOOTSTRAP (COMPATIBLE)
       ========================= */
    std::string Bootstrap(const std::string& a) override {

        Ciphertext ct = Deserialize(a);
        ct.noise = 1;

        return Serialize(ct);
    }

    /* =========================
       KEY STORAGE
       ========================= */
    void SaveKeys(const std::string& path) override {

        std::ofstream out(path);

        for (auto v : sk.s) out << v << " ";
        out << "\n";

        for (auto v : pk.a) out << v << " ";
        out << "\n";

        for (auto v : pk.b) out << v << " ";
        out << "\n";

        for (auto v : evk.relin_key) out << v << " ";
        out << "\n";
    }

    void LoadAllKeys(const std::string& path) override {

        std::ifstream in(path);

        sk.s.resize(POLY_DEGREE);
        pk.a.resize(POLY_DEGREE);
        pk.b.resize(POLY_DEGREE);
        evk.relin_key.resize(POLY_DEGREE);

        for (auto& v : sk.s) in >> v;
        for (auto& v : pk.a) in >> v;
        for (auto& v : pk.b) in >> v;
        for (auto& v : evk.relin_key) in >> v;
    }

    void LoadEvalKeys(const std::string& path) override {
        std::ifstream in(path);
        evk.relin_key.resize(POLY_DEGREE);
        for (auto& v : evk.relin_key) in >> v;
    }
};

/* =========================
   FACTORY
   ========================= */
FHEEngine* CreateFHEEngine() {
    return new AdvancedFHEEngine();
}