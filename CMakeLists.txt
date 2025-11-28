cmake_minimum_required(VERSION 3.16)
project(s57-postgis VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GDAL REQUIRED gdal)
pkg_check_modules(PQXX REQUIRED libpqxx)

# Source files
set(SOURCES
    src/main.cpp
    src/s57.cpp
    src/database.cpp
    src/ingest.cpp
    src/json_utils.cpp
)

# Headers
set(HEADERS
    src/types.hpp
    src/zfinder.hpp
    src/s57.hpp
    src/database.hpp
    src/ingest.hpp
    src/json_utils.hpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${GDAL_INCLUDE_DIRS}
    ${PQXX_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${GDAL_LIBRARIES}
    ${PQXX_LIBRARIES}
    pthread
)

# Add library directories
target_link_directories(${PROJECT_NAME} PRIVATE
    ${GDAL_LIBRARY_DIRS}
    ${PQXX_LIBRARY_DIRS}
)

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Installation
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Install schema
install(FILES sql/schema.sql
    DESTINATION share/${PROJECT_NAME}
)

# Install setup script
install(PROGRAMS scripts/setup_db.sh
    DESTINATION share/${PROJECT_NAME}
)
