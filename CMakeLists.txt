cmake_minimum_required(VERSION 3.16)
project(ShieldedParquet CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)

find_package(OpenFHE REQUIRED)

include_directories(${OpenFHE_INCLUDE})

add_executable(ShieldedParquet 
    src/main.cpp 
    src/core/crypto_manager.cpp
)

target_link_libraries(ShieldedParquet
    PRIVATE
    Arrow::arrow_shared
    Parquet::parquet_shared
    ${OpenFHE_LIBRARIES}
)