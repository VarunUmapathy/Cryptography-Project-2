import os
import math
import pandas as pd
import pyarrow.parquet as pq

class FHESecurityEstimator:
    """
    Estimates security metrics based on the Homomorphic Encryption Standard
    for RLWE-based schemes (like BFV).
    """
    def __init__(self, ring_dimension, log_q, plaintext_modulus):
        self.N = ring_dimension
        self.log_Q = log_q
        self.t = plaintext_modulus
        
    def estimate_security_level(self):
        """
        Estimates the bits of security (lambda) against known lattice attacks
        (like the Dual Attack or Primal uSVP).
        Based on the HE Standard tables for ternary/error variance.
        """
        # A simplified heuristic of the LWE estimator: 
        # Security depends heavily on the ratio of N to log_Q.
        ratio = self.N / self.log_Q
        
        if ratio > 37.5:
            return "256-bit (Post-Quantum Secure)"
        elif ratio > 30.0:
            return "192-bit (Post-Quantum Secure)"
        elif ratio > 20.0:
            return "128-bit (Standard Secure)"
        else:
            return "INSECURE (Below 128-bit)"

    def eval_add_noise_growth(self):
        """
        In BFV, EvalAdd increases the invariant noise. 
        Adding two ciphertexts roughly doubles the noise magnitude.
        """
        # Noise grows additively. 1 EvalAdd = ~1 bit of noise budget consumed.
        return "1 bit per EvalAdd operation"

def analyze_parquet_metrics(file_path, original_plaintext_bytes_per_val=8):
    """
    Analyzes the physical metrics of the Shielded Parquet file.
    """
    if not os.path.exists(file_path):
        print(f"Error: File '{file_path}' not found.")
        return

    # 1. File Level Metrics
    total_file_size = os.path.getsize(file_path)
    print("="*50)
    print(f"📊 SHIELDED PARQUET METRICS: {os.path.basename(file_path)}")
    print("="*50)
    print(f"Total File Size:      {total_file_size / 1024:.2f} KB")

    # 2. Read the Parquet File
    table = pq.read_table(file_path)
    df = table.to_pandas()
    num_rows = len(df)
    print(f"Total Rows:           {num_rows}")

    # 3. Column Level Analysis (AES vs FHE)
    print("\n--- Column Storage Breakdown ---")
    fhe_cols = []
    
    for col in df.columns:
        # Calculate the total bytes used by this column
        # Arrow stores binary data as Python bytes objects
        col_bytes = df[col].apply(lambda x: len(x) if isinstance(x, bytes) else len(str(x).encode())).sum()
        avg_bytes_per_row = col_bytes / num_rows if num_rows > 0 else 0
        
        # Heuristic: FHE ciphertexts are massive (usually > 100KB per value)
        # AES-GCM tags + ciphertext are small (usually < 100 Bytes)
        if avg_bytes_per_row > 10000:
            enc_type = "FHE (BFV Polynomial)"
            fhe_cols.append((col, avg_bytes_per_row))
        elif isinstance(df[col].iloc[0], bytes):
            enc_type = "AES-256-GCM"
        else:
            enc_type = "Plaintext/Unknown"

        print(f"Column: {col:<15} | Type: {enc_type:<20} | Avg Size: {avg_bytes_per_row:,.0f} Bytes/row")

    # 4. Ciphertext Expansion Factor
    print("\n--- Expansion Metrics ---")
    if fhe_cols:
        for col_name, avg_size in fhe_cols:
            expansion_factor = avg_size / original_plaintext_bytes_per_val
            print(f"[{col_name}] Expansion Factor: {expansion_factor:,.1f}x (compared to {original_plaintext_bytes_per_val}-byte integer)")
    else:
        print("No FHE columns detected.")

if __name__ == "__main__":
    # --- 1. Analyze the Storage File ---
    # Point this to your generated hybrid Parquet file
    TARGET_FILE = "hybrid.parquet" 
    
    # Create a dummy file for demonstration if it doesn't exist
    if not os.path.exists(TARGET_FILE):
        print("Note: hybrid.parquet not found. Creating a simulated DataFrame for demonstration...")
        df_sim = pd.DataFrame({
            "EmployeeName": [os.urandom(32) for _ in range(5)], # Simulated AES (32 bytes)
            "BasePay": [os.urandom(131072) for _ in range(5)]   # Simulated BFV (128 KB)
        })
        df_sim.to_parquet(TARGET_FILE)

    analyze_parquet_metrics(TARGET_FILE)

    # --- 2. Analyze the Cryptographic Security ---
    print("\n" + "="*50)
    print("🛡️ CRYPTOGRAPHIC SECURITY METRICS (BFV Scheme)")
    print("="*50)
    
    # Replace these with the actual parameters from your OpenFHE C++ code
    # Typical standard parameters for exact arithmetic:
    RING_DIMENSION = 8192       # N
    LOG_Q_MODULUS = 218         # q (in bits)
    PLAINTEXT_MODULUS = 65537   # t
    
    security_analyzer = FHESecurityEstimator(RING_DIMENSION, LOG_Q_MODULUS, PLAINTEXT_MODULUS)
    
    print(f"Ring Dimension (N):   {RING_DIMENSION}")
    print(f"Ciphertext Modulus:   {LOG_Q_MODULUS}-bit")
    print(f"Plaintext Modulus(t): {PLAINTEXT_MODULUS}")
    print("-" * 50)
    print(f"Lattice Security:     {security_analyzer.estimate_security_level()}")
    print(f"Noise Growth (Add):   {security_analyzer.eval_add_noise_growth()}")
    print("="*50)