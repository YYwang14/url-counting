# URL Processing Project

## Overview
This project consists of three stages aimed at efficient URL counting and processing for large datasets. It includes:

1. **URL Generation**: A Python script (`url_generation.py`) generates a dataset of random URLs up to a specified size for testing purposes.
2. **Baseline Implementation**: A baseline C++ program (`baseline.cpp`) for counting the frequency of URLs.
3. **Revised and Optimized Versions**: Two improved implementations:
    - `revised.cpp`: An intermediate version with better code organization and improvements over the baseline.
    - `url_counting.cpp`: A highly optimized version focusing on reduced runtime at the cost of increased memory usage.

## Environment
This project was developed and tested on the following setup:
- **Platform**: Oracle VirtualBox running Ubuntu 24.4
- **CPU**: 16 CPUs allocated
- **Memory**: 16 GB
- **Disk**: NVMe WD PC SN560

## Files

### 1. URL Generation
- **File**: `url_generation.py`
- **Description**: Generates a large file of random URLs for testing.
- **Usage**:
  ```bash
  python url_generation.py
  ```
  - Default filename: `urls.txt`
  - Default maximum number of distinct URLs: `1000`
  - Default maximum size: `10 GB`

### 2. Baseline Implementation
- **File**: `baseline.cpp`
- **Description**: A simple implementation for counting URL frequencies.
- **Features**:
  - Reads the input file line by line.
  - Uses basic data structures for counting.
  - Suitable for smaller datasets but limited in performance for large datasets.
- **Compilation**:
  ```bash
  g++ -std=c++17 baseline.cpp -o baseline 
  ```
- **Execution**:
  ```bash
  ./baseline
  ```

### 3. Revised Implementation
- **File**: `revised.cpp`
- **Description**: Improves upon the baseline with better organization and minor performance enhancements.
- **Changes from Baseline**:
  - Cleaner code structure.
  - Line-by-line processing retained but optimized slightly for clarity.
- **Compilation**:
  ```bash
  g++ -std=c++17 revised.cpp -o revised
  ```
- **Execution**:
  ```bash
  ./revised
  ```

### 4. Optimized Implementation
- **File**: `url_counting.cpp`
- **Description**: Introduces a highly optimized implementation with faster processing for large datasets.
- **Key Features**:
  - Uses buffered I/O to minimize disk access.
  - Processes URLs in chunks to improve efficiency.
  - Trades higher memory usage for faster runtime.
- **Compilation and Execution**:
  ```bash
  g++ -std=c++17 url_counting.cpp -o url_counting
- **Execution**:
  ```bash
  ./url_counting
  ```  ```

## Comparison

### Performance
| Version          | Memory Usage | Runtime   | Key Advantages                          |
|------------------|--------------|-----------|-----------------------------------------|
| `baseline.cpp`   | Low          | Slow      | Simple and easy to understand.          |
| `revised.cpp`    | low          | Moderate  | Improved organization and readability.  |
| `url_counting.cpp` | High         | Fast      | Highly efficient for large datasets.    |

### Recommendations
- Use `baseline.cpp` for small datasets or when simplicity is key.
- Use `url_counting.cpp` for large datasets where performance is critical.


