#pragma once
#include <chrono>

class Timer {
private:
    std::chrono::high_resolution_clock::time_point start;

public:
    void Start() {
        start = std::chrono::high_resolution_clock::now();
    }

    double Stop() {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;
        return diff.count(); // seconds
    }
};