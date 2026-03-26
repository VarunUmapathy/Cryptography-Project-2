cmake_minimum_required(VERSION 3.16)
project(ShieldedParquet)

set(CMAKE_CXX_STANDARD 17)

find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(ShieldedParquet
    src/main.cpp
    src/core/crypto_manager.cpp
    src/core/csv_ingester.cpp
    src/core/parquet_handler.cpp
    src/core/schema_parser.cpp
    src/core/simple_fhe_engine.cpp
)

target_link_libraries(ShieldedParquet
    Arrow::arrow_shared
    Parquet::parquet_shared
    OpenSSL::Crypto
)