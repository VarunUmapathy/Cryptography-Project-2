sudo apt update
sudo apt install build-essential cmake gdb libssl-dev nlohmann-json3-dev
sudo apt install libarrow-dev libparquet-dev
sudo apt install python3-pip
pip3 install fastapi uvicorn python-multipart
mkdir -p build && cd build
rm -rf * # Ensure a clean build
cmake ..
make -j$(nproc)