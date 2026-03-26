# Shielded Parquet Project – Setup & Execution Guide

---

## Prerequisites

Install the following:

### System Dependencies (Linux / WSL)
```bash
sudo apt update
sudo apt install build-essential cmake libssl-dev
sudo apt install libarrow-dev libparquet-dev
```

### Python Dependancies
```
pip install -r requirements.txt
```

### Build the Engine from Project Root

```
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Run the backend server
```
uvicorn server:app --reload
```