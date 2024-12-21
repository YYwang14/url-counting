#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <stdexcept>
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
                return usage.ru_maxrss * 1024; // Convert KB to bytes
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

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    return ss.str();
}

std::string getLogTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void logMessage(std::ofstream& logFile, const std::string& message) {
    std::string timestamp = getLogTimestamp();
    std::string memUsage = MemoryMonitor::formatMemoryUsage(MemoryMonitor::getCurrentMemoryUsage());
    std::string logMessage = "[" + timestamp + "][Memory: " + memUsage + "] " + message;
    logFile << logMessage << std::endl;
    std::cout << logMessage << std::endl;
}

int main() {
    auto startTime = std::chrono::steady_clock::now();
    
    // Open log file with timestamp
    std::string logFilename = "baseline_" + getCurrentTimestamp() + ".log";
    std::ofstream logFile(logFilename);
    if (!logFile) {
        std::cerr << "Failed to open log file: " << logFilename << std::endl;
        return 1;
    }

    try {
        logMessage(logFile, "Starting URL counting process");

        // Read all URLs into a hash map
        std::unordered_map<std::string, int> urlFreq;
        std::ifstream inFile("urls.txt");
        if (!inFile) {
            throw std::runtime_error("Failed to open input file: urls.txt");
        }

        std::string url;
        size_t lineCount = 0;
        while (std::getline(inFile, url)) {
            if (!url.empty()) {
                urlFreq[url]++;
                lineCount++;
                if (lineCount % 1000000 == 0) {
                    logMessage(logFile, "Processed " + std::to_string(lineCount) + " lines");
                }
            }
        }
        logMessage(logFile, "Finished reading input file. Total lines: " + std::to_string(lineCount));

        // Convert to vector for sorting
        std::vector<std::pair<std::string, int>> results;
        results.reserve(urlFreq.size());
        for (const auto& [url, count] : urlFreq) {
            results.push_back({url, count});
        }
        logMessage(logFile, "Converted to vector. Unique URLs: " + std::to_string(results.size()));

        // Sort by count in descending order
        logMessage(logFile, "Starting sort");
        std::sort(results.begin(), results.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        logMessage(logFile, "Finished sorting");

        // Write top 100 to file
        std::string resultFilename = "results_" + getCurrentTimestamp() + ".txt";
        std::ofstream outFile(resultFilename);
        if (!outFile) {
            throw std::runtime_error("Failed to create output file: " + resultFilename);
        }

        logMessage(logFile, "Writing results to " + resultFilename);
        outFile << "Rank\tURL\tCount\n";
        const int TOP_N = 100;
        for (int i = 0; i < std::min(TOP_N, (int)results.size()); i++) {
            outFile << (i + 1) << "\t" << results[i].first << "\t" << results[i].second << "\n";
            logMessage(logFile, std::to_string(i + 1) + ". " + results[i].first + 
                      ": " + std::to_string(results[i].second));
        }

        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();
        logMessage(logFile, "Process completed. Total time: " + std::to_string(duration) + " seconds");

    } catch (const std::exception& e) {
        logMessage(logFile, "Error: " + std::string(e.what()));
        return 1;
    }

    return 0;
}