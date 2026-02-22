cmake_minimum_required(VERSION 3.16)
project(ShieldedParquet CXX)

set(CMAKE_CXX_STANDARD 17)

# Tell CMake to look inside your active Conda environment
if(DEFINED ENV{CONDA_PREFIX})
    message(STATUS "Using Conda environment: $ENV{CONDA_PREFIX}")
    list(APPEND CMAKE_PREFIX_PATH "$ENV{CONDA_PREFIX}")
endif()

# Now CMake can find Arrow and Parquet from Conda
find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)

# (You will still need to find OpenFHE here once you build it)

add_executable(ShieldedParquet src/main.cpp src/core/crypto_manager.cpp)

target_link_libraries(ShieldedParquet
    PRIVATE
    Arrow::arrow_shared
    Parquet::parquet_shared
)