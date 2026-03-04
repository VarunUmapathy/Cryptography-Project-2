from fastapi import FastAPI, UploadFile, File, HTTPException
from fastapi.responses import JSONResponse, FileResponse
from fastapi.staticfiles import StaticFiles
import subprocess
import os
import shutil

app = FastAPI(title="Shielded-Parquet API")

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
TEMP_DIR = os.path.join(BASE_DIR, "temp_data")
CPP_EXEC = os.path.join(BASE_DIR, "build", "ShieldedParquet")

os.makedirs(TEMP_DIR, exist_ok=True)

app.mount("/static", StaticFiles(directory=os.path.join(BASE_DIR, "static")), name="static")

@app.get("/")
async def serve_frontend():
    """Serves the main HTML Web UI."""
    return FileResponse(os.path.join(BASE_DIR, "static", "index.html"))

def run_cpp_engine(args: list) -> dict:
    command = [CPP_EXEC] + args
    print(f"Executing: {' '.join(command)}")
    
    try:
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        
        if result.returncode != 0:
            print(f"C++ Error:\n{result.stderr}")
            raise HTTPException(status_code=500, detail=f"C++ Engine Failed: {result.stderr}")
            
        return {"status": "success", "output": result.stdout}
        
    except FileNotFoundError:
        raise HTTPException(status_code=500, detail="C++ executable not found. Did you run 'make' in the build folder?")

@app.post("/api/upload")
async def upload_and_ingest(csv_file: UploadFile = File(...), schema_file: UploadFile = File(...)):
    """Receives files from the UI and triggers the 'ingest' phase."""
    csv_path = os.path.join(TEMP_DIR, "data.csv")
    schema_path = os.path.join(TEMP_DIR, "schema.json")
    out_parquet = os.path.join(TEMP_DIR, "hybrid.parquet")
    
    # Save the uploaded files to disk
    with open(csv_path, "wb") as buffer:
        shutil.copyfileobj(csv_file.file, buffer)
    with open(schema_path, "wb") as buffer:
        shutil.copyfileobj(schema_file.file, buffer)
        
    # Call: ./ShieldedParquet ingest temp_data/data.csv temp_data/schema.json temp_data/hybrid.parquet
    result = run_cpp_engine(["ingest", csv_path, schema_path, out_parquet])
    return {"message": "Files ingested and encrypted successfully", "log": result["output"]}

@app.post("/api/compute")
async def simulate_cloud_compute():
    """Triggers the 'compute' phase (Cloud adding the bonus)."""
    in_parquet = os.path.join(TEMP_DIR, "hybrid.parquet")
    out_parquet = os.path.join(TEMP_DIR, "hybrid_computed.parquet")
    
    # Call: ./ShieldedParquet compute temp_data/hybrid.parquet temp_data/hybrid_computed.parquet
    result = run_cpp_engine(["compute", in_parquet, out_parquet])
    return {"message": "Cloud computation complete", "log": result["output"]}

@app.get("/api/results")
async def decrypt_results():
    """Triggers the 'decrypt' phase and returns the results."""
    in_parquet = os.path.join(TEMP_DIR, "hybrid_computed.parquet")
    
    # Call: ./ShieldedParquet decrypt temp_data/hybrid_computed.parquet
    result = run_cpp_engine(["decrypt", in_parquet])
    return {"message": "Decryption complete", "data": result["output"]}

# Optional: Mount a static folder if you want FastAPI to host your HTML/JS directly
# os.makedirs(os.path.join(BASE_DIR, "static"), exist_ok=True)
# app.mount("/", StaticFiles(directory="static", html=True), name="static")