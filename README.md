# S57-PostGIS

A complete C++ port of the [Njord](https://github.com/manimaul/njord) S-57 chart processing system that imports Electronic Navigational Charts (ENC) into a PostGIS database.

## Overview

This project faithfully recreates Njord's S-57 processing logic in C++. It processes S-57 format nautical chart files and stores them in a PostGIS database using the exact same schema and logic as Njord.

### Features

- Process S-57 (.000) format nautical chart files
- Store chart data in PostGIS with full spatial indexing
- Extract chart metadata (DSID, M_COVR)
- Handle all S-57 layers including special SOUNDG depth handling
- Calculate zoom levels from chart scale
- Batch processing with progress reporting
- Docker support with PostGIS database

## Building

### Prerequisites

- CMake 3.16+
- C++17 compiler (GCC 8+, Clang 7+)
- GDAL (libgdal-dev)
- libpqxx (PostgreSQL C++ client)
- PostgreSQL with PostGIS extension

### Ubuntu/Debian

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libgdal-dev \
    libpqxx-dev \
    libpq-dev \
    gdal-bin \
    postgresql-client

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### macOS

```bash
# Install dependencies
brew install cmake gdal libpqxx

# Build
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

## Usage

```
Usage: s57-postgis <input> [options]

Input:
  <input>                 S-57 file (.000) or directory

Database Options:
  -d, --database <conn>   PostgreSQL connection string
                          Default: postgresql://localhost/njord
  --init-schema           Initialize database schema

Processing Options:
  -w, --workers <n>       Number of parallel workers (default: 4)
  -r, --recursive         Recursively search directories
  -v, --verbose           Verbose output

Other Options:
  --list                  List all .000 files found
  --info                  Show chart metadata (for single file)
  -h, --help              Show help
  --version               Show version
```

### Examples

```bash
# Initialize database and process a single chart
./s57-postgis chart.000 -d postgresql://localhost/njord --init-schema

# Process all charts in a directory recursively with verbose output
./s57-postgis /path/to/charts -r -v

# List all S-57 files in a directory
./s57-postgis /path/to/charts --list -r

# Show metadata for a specific chart
./s57-postgis chart.000 --info
```

## Docker

### Quick Start

```bash
cd docker

# Start PostGIS database
docker-compose up -d postgis

# Build and run the application
docker-compose up s57-postgis
```

### Manual Build

```bash
# Build the image
docker build -t s57-postgis -f docker/Dockerfile .

# Run with volume mount for chart files
docker run --rm \
  -v /path/to/charts:/charts:ro \
  --network host \
  s57-postgis /charts -d postgresql://localhost/njord -r
```

## Database Schema

The database schema matches Njord's schema exactly:

### Tables

- **meta**: Schema version tracking
- **charts**: Chart metadata with coverage geometry
- **features**: All chart features with geometry and properties

### Indexes

- GIST indexes on geometries for spatial queries
- B-tree indexes on primary keys and layer names
- GIN index on LNAM references
- GIST index on zoom range for scale filtering

See [sql/schema.sql](sql/schema.sql) for the complete schema.

## Architecture

### Key Components

| File | Description |
|------|-------------|
| `src/s57.hpp/cpp` | S-57 file processing using GDAL |
| `src/database.hpp/cpp` | PostGIS database operations |
| `src/ingest.hpp/cpp` | Batch chart processing |
| `src/zfinder.hpp` | Zoom level calculation from scale |
| `src/json_utils.hpp/cpp` | JSON serialization utilities |
| `src/types.hpp` | Common types and structures |
| `src/main.cpp` | CLI entry point |

### GDAL Options

The following GDAL S-57 options are used (matching Njord):

```
RETURN_PRIMITIVES=OFF,RETURN_LINKAGES=OFF,LNAM_REFS=ON,UPDATES=APPLY,
SPLIT_MULTIPOINT=ON,RECODE_BY_DSSI=ON:ADD_SOUNDG_DEPTH=ON
```

### Excluded Layers

The following S-57 layers are excluded from feature processing:

- DSID (Data Set Identification)
- IsolatedNode
- ConnectedNode
- Edge
- Face

## Relationship to Njord

This project is a C++ port of the chart processing functionality from [manimaul/njord](https://github.com/manimaul/njord). The goal is to provide identical functionality with the same database schema, making it compatible with Njord's tile server.

### Ported Components

- `S57.kt` → `src/s57.cpp`
- `ChartDao.kt` / `GeoJsonDao.kt` → `src/database.cpp`
- `ChartIngest.kt` → `src/ingest.cpp`
- `ZFinder.kt` → `src/zfinder.hpp`
- `up.sql` → `sql/schema.sql`

## License

Apache 2.0 - Same as Njord. See [LICENSE](LICENSE) for details.

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## Acknowledgments

- [Njord](https://github.com/manimaul/njord) - The original Kotlin/Java implementation
- [GDAL](https://gdal.org/) - Geospatial Data Abstraction Library
- [PostGIS](https://postgis.net/) - Spatial database extender for PostgreSQL
