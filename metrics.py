import os
import time
import subprocess
import pandas as pd

def run_benchmarks(raw_csv_path, schema_path):
    print("="*60)
    print("🚀 SHIELDED PARQUET BENCHMARKING SUITE")
    print("="*60)
    
    # 1. Baseline: Get raw data size
    raw_size_mb = os.path.getsize(raw_csv_path) / (1024 * 1024)
    print(f"Dataset Size: {raw_size_mb:.4f} MB")

    # --- METRIC 1: ENCRYPTION & WRITE LATENCY ---
    print("\n[1/3] Running Client Ingestion (AES + FHE Encryption)...")
    start_time = time.perf_counter()
    
    # Simulating your C++ subprocess call for upload/ingest
    subprocess.run(["./build/ShieldedParquet", "ingest", raw_csv_path, schema_path], capture_output=True)
    
    encrypt_time = time.perf_counter() - start_time
    throughput_mb_s = raw_size_mb / encrypt_time if encrypt_time > 0 else 0
    print(f"⏱️  Encryption Time:    {encrypt_time:.3f} seconds")
    print(f"⚡ Throughput:         {throughput_mb_s:.3f} MB/s")

    # --- METRIC 2: STORAGE OVERHEAD ---
    hybrid_file = "temp/hybrid.parquet"
    if os.path.exists(hybrid_file):
        enc_size_mb = os.path.getsize(hybrid_file) / (1024 * 1024)
        overhead_percent = ((enc_size_mb - raw_size_mb) / raw_size_mb) * 100
        print(f"💾 Encrypted Size:     {enc_size_mb:.4f} MB (Overhead: {overhead_percent:.1f}%)")

    # --- METRIC 3: CLOUD COMPUTE LATENCY ---
    print("\n[2/3] Running Cloud Computation (FHE EvalAdd)...")
    start_time = time.perf_counter()
    
    # Simulating your C++ subprocess call for compute
    subprocess.run(["./build/ShieldedParquet", "compute", hybrid_file], capture_output=True)
    
    compute_time = time.perf_counter() - start_time
    print(f"⏱️  Blind Compute Time: {compute_time:.3f} seconds")

    # --- METRIC 4: DECRYPTION & VERIFICATION ---
    computed_file = "temp/hybrid_computed.parquet"
    print("\n[3/3] Running Client Decryption...")
    start_time = time.perf_counter()
    
    # Simulating your C++ subprocess call for decrypt
    subprocess.run(["./build/ShieldedParquet", "decrypt", computed_file], capture_output=True)
    
    decrypt_time = time.perf_counter() - start_time
    print(f"⏱️  Decryption Time:    {decrypt_time:.3f} seconds")

    # --- SUMMARY REPORT ---
    print("\n" + "="*60)
    print("📊 FORMAL METRICS SUMMARY (For Research Paper)")
    print("="*60)
    print(f"End-to-End Pipeline Latency:  {encrypt_time + compute_time + decrypt_time:.3f} seconds")
    print(f"FHE Compute Penalty vs AES:   (Compare {compute_time:.3f}s to standard CPU arithmetic)")
    print("="*60)

if __name__ == "__main__":
    # Point this to your test files
    run_benchmarks("temp_data/data.csv", "temp_data/schema.json")