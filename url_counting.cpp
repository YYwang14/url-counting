#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#ifdef __linux__
    #include <sys/resource.h>
#elif _WIN32
    #include <windows.h>
    #include <psapi.h>
#endif

class MemoryMonitor {
   public:
    static size_t getCurrentMemoryUsage() {
#ifdef __linux__
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            return usage.ru_maxrss * 1024;  // Convert KB to bytes
        }
        return 0;
#elif _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            return pmc.WorkingSetSize;
        }
        return 0;
#else
        return 0;
#endif
    }

    static std::string formatMemoryUsage(size_t bytes) {
        return std::to_string(bytes / (1024 * 1024)) + "MB";
    }
};

void logMessage(std::ofstream& logFile, const std::string& message, std::mutex& logMutex) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");

    std::string memUsage = MemoryMonitor::formatMemoryUsage(MemoryMonitor::getCurrentMemoryUsage());
    std::string logMessage = "[" + ss.str() + "][Memory: " + memUsage + "] " + message;

    std::lock_guard<std::mutex> lock(logMutex);
    logFile << logMessage << std::endl;
    std::cout << logMessage << std::endl;
}

void processChunk(const std::string& filename, std::streampos start, std::streampos end,
                  std::unordered_map<std::string, int>& freq, std::mutex& mapMutex,
                  std::ofstream& logFile, std::mutex& logMutex) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Failed to open file in thread");
    }

    file.seekg(start);

    // If not the first chunk, find the start of next line
    if (start > 0) {
        std::string partial;
        std::getline(file, partial);
    }

    std::unordered_map<std::string, int> localFreq;
    std::string url;
    size_t lineCount = 0;
    std::vector<char> buffer(1024 * 1024);

    while (end - file.tellg() > 1024 * 1024) {
        file.read(buffer.data(), buffer.size());
        std::string tmp;
        std::getline(file, tmp);
        std::istringstream bufferStream(std::string(buffer.data(), buffer.size()) + tmp);
        while (!bufferStream.eof() && std::getline(bufferStream, url)) {
            if (!url.empty()) {
                localFreq[url]++;
                lineCount++;
                if (lineCount % 1000000 == 0) {
                    logMessage(logFile, "Processed " + std::to_string(lineCount) + " lines",
                               logMutex);
                }
            }
        }
    }
    while (file.tellg() < end && std::getline(file, url)) {
        if (!url.empty()) {
            localFreq[url]++;
            lineCount++;
            if (lineCount % 1000000 == 0) {
                logMessage(logFile, "Processed " + std::to_string(lineCount) + " lines",
                            logMutex);
            }
        }
    }


    // Merge local results into global map
    std::lock_guard<std::mutex> lock(mapMutex);
    for (const auto& [url, count] : localFreq) {
        freq[url] += count;
    }
}

int main() {
    auto startTime = std::chrono::steady_clock::now();

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&timestamp), "%Y%m%d_%H%M%S");

    std::string logFilename = "counting_" + ss.str() + ".log";
    std::ofstream logFile(logFilename);
    if (!logFile) {
        std::cerr << "Failed to open log file: " << logFilename << std::endl;
        return 1;
    }

    std::mutex logMutex;
    std::mutex mapMutex;

    try {
        const size_t NUM_THREADS = 8;
        std::string filename = "urls.txt";

        // Get file size
        std::ifstream sizeCheck(filename, std::ios::ate);
        if (!sizeCheck) {
            throw std::runtime_error("Failed to open input file: " + filename);
        }
        std::streampos fileSize = sizeCheck.tellg();
        sizeCheck.close();

        logMessage(logFile, "Starting URL counting process", logMutex);

        // Calculate chunk sizes
        std::streampos chunkSize = fileSize / NUM_THREADS;
        std::vector<std::streampos> chunkStarts(NUM_THREADS);
        std::vector<std::streampos> chunkEnds(NUM_THREADS);

        for (size_t i = 0; i < NUM_THREADS; ++i) {
            chunkStarts[i] = i * chunkSize;
            chunkEnds[i] = (i == NUM_THREADS - 1) ? fileSize : chunkStarts[i] + chunkSize;
        }

        std::unordered_map<std::string, int> totalFreq;
        std::vector<std::thread> threads;

        // Start threads
        for (size_t i = 0; i < NUM_THREADS; ++i) {
            threads.emplace_back(processChunk, filename, chunkStarts[i], chunkEnds[i],
                                 std::ref(totalFreq), std::ref(mapMutex), std::ref(logFile),
                                 std::ref(logMutex));
        }

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }

        logMessage(logFile, "Converting to vector for sorting", logMutex);
        std::vector<std::pair<std::string, int>> results;
        results.reserve(totalFreq.size());
        for (const auto& [url, count] : totalFreq) {
            results.push_back({url, count});
        }

        logMessage(logFile, "Sorting results", logMutex);
        std::sort(results.begin(), results.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        std::string resultFilename = "counting_results_" + ss.str() + ".txt";
        std::ofstream outFile(resultFilename);
        if (!outFile) {
            throw std::runtime_error("Failed to create result file");
        }

        logMessage(logFile, "Writing results to " + resultFilename, logMutex);
        outFile << "Rank\tURL\tCount\n";
        const int TOP_N = 100;
        for (int i = 0; i < std::min(TOP_N, (int)results.size()); i++) {
            outFile << (i + 1) << "\t" << results[i].first << "\t" << results[i].second << "\n";
        }

        auto endTime = std::chrono::steady_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();
        logMessage(logFile,
                   "Process completed. Total time: " + std::to_string(duration) + " seconds",
                   logMutex);

    } catch (const std::exception& e) {
        logMessage(logFile, "Error: " + std::string(e.what()), logMutex);
        return 1;
    }

    return 0;
}