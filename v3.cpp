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
#include <queue>
#include <functional>

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
        double mb = bytes / (1024.0 * 1024.0);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << mb << " MB";
        return oss.str();
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

        // Estimate number of unique URLs to reserve space in unordered_map
        // For example, if you expect up to 10 million unique URLs:
        const size_t EXPECTED_UNIQUE_URLS = 10000000;
        std::unordered_map<std::string, int> urlFreq;
        urlFreq.reserve(EXPECTED_UNIQUE_URLS);
        logMessage(logFile, "Reserved space for unordered_map");

        // Read URLs and count frequencies
        std::ifstream inFile("urls.txt");
        if (!inFile) {
            throw std::runtime_error("Failed to open input file: urls.txt");
        }

        std::string url;
        size_t lineCount = 0;
        while (std::getline(inFile, url)) {
            if (!url.empty()) {
                ++urlFreq[url];
                ++lineCount;
                if (lineCount % 1000000 == 0) {
                    logMessage(logFile, "Processed " + std::to_string(lineCount) + " lines");
                }
            }
        }
        logMessage(logFile, "Finished reading input file. Total lines: " + std::to_string(lineCount));

        // Use a min-heap to keep track of top 100 URLs
        logMessage(logFile, "Starting to determine top 100 URLs using a min-heap");

        // Define a min-heap where the smallest count is on top
        std::priority_queue<
            std::pair<std::string, int>,
            std::vector<std::pair<std::string, int>>,
            std::function<bool(const std::pair<std::string, int>&, const std::pair<std::string, int>&)>
        > minHeap(
            [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                return a.second > b.second; // Min-heap based on count
            }
        );

        for (const auto& [url, count] : urlFreq) {
            if (minHeap.size() < 100) {
                minHeap.emplace(url, count);
            } else if (count > minHeap.top().second) {
                minHeap.pop();
                minHeap.emplace(url, count);
            }
        }

        logMessage(logFile, "Finished determining top 100 URLs");

        // Extract the top 100 URLs from the heap into a vector
        std::vector<std::pair<std::string, int>> topUrls;
        topUrls.reserve(100);
        while (!minHeap.empty()) {
            topUrls.emplace_back(minHeap.top());
            minHeap.pop();
        }

        // Sort the top URLs in descending order
        std::sort(topUrls.begin(), topUrls.end(),
                  [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                      return b.second < a.second; // Descending order
                  });

        logMessage(logFile, "Top 100 URLs determined");

        // Write top 100 to file
        std::string resultFilename = "results_" + getCurrentTimestamp() + ".txt";
        std::ofstream outFile(resultFilename);
        if (!outFile) {
            throw std::runtime_error("Failed to create output file: " + resultFilename);
        }

        logMessage(logFile, "Writing results to " + resultFilename);
        outFile << "Rank\tURL\tCount\n";
        for (size_t i = 0; i < topUrls.size(); ++i) {
            outFile << (i + 1) << "\t" << topUrls[i].first << "\t" << topUrls[i].second << "\n";
            logMessage(logFile, std::to_string(i + 1) + ". " + topUrls[i].first + 
                      ": " + std::to_string(topUrls[i].second));
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
