# S57-PostGIS Build Image
# C++ port of Njord's S-57 chart processing system

FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libgdal-dev \
    libpqxx-dev \
    libpq-dev \
    gdal-bin \
    postgresql-client \
    && rm -rf /var/lib/apt/lists/*

# Set work directory
WORKDIR /app

# Copy source files
COPY CMakeLists.txt .
COPY src/ src/

# Build the application
RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Set the entrypoint
ENTRYPOINT ["/app/build/s57-postgis"]
