cmake_minimum_required(VERSION 3.16)
project(ShieldedParquet CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)
find_package(OpenSSL REQUIRED)

# Define the executable
add_executable(ShieldedParquet 
    src/main.cpp 
    src/core/crypto_manager.cpp
    src/core/parquet_handler.cpp
    src/core/schema_parser.cpp
    src/core/csv_ingester.cpp
)

# 1. Provide the DIRECTORIES where the header files live
target_include_directories(ShieldedParquet PRIVATE 
    /usr/local/include/openfhe
    /usr/local/include/openfhe/core
    /usr/local/include/openfhe/pke
    /usr/local/include/openfhe/binfhe
    /usr/local/include/openfhe/third-party/include
    /usr/local/include/openfhe/core/metadata
)

# 2. Provide the exact .so files where the math logic lives
target_link_libraries(ShieldedParquet
    PRIVATE
    Arrow::arrow_shared
    Parquet::parquet_shared
    OpenSSL::Crypto
    /usr/local/lib/libOPENFHEpke.so
    /usr/local/lib/libOPENFHEcore.so
    /usr/local/lib/libOPENFHEbinfhe.so
)