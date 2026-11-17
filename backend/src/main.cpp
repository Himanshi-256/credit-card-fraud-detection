#include "server.h"
#include <iostream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [options]\n"
              << "Options:\n"
              << "  --port <number>    Set server port (default: 8080)\n"
              << "  --db <path>        Path to SQLite database file (default: database/fraud_detection.db)\n"
              << "  --csv <path>       Path to sample CSV dataset (default: dataset/transactions.csv)\n"
              << "  --no-seed          Disable auto-seeding sample dataset\n"
              << "  --help, -h         Show this help message\n";
}

int main(int argc, char* argv[]) {
    ServerConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            config.port = std::stoi(argv[++i]);
        } else if (arg == "--db" && i + 1 < argc) {
            config.dbPath = argv[++i];
        } else if (arg == "--csv" && i + 1 < argc) {
            config.csvPath = argv[++i];
        } else if (arg == "--no-seed") {
            config.seedSampleData = false;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    // Ensure database directory exists
    try {
        fs::path dbPath(config.dbPath);
        if (dbPath.has_parent_path()) {
            fs::create_directories(dbPath.parent_path());
        }
    } catch (const std::exception& e) {
        std::cerr << "[Warning] Could not create database directory: " << e.what() << std::endl;
    }

    Server server(config);
    if (!server.init()) {
        std::cerr << "[Fatal] Server initialization failed." << std::endl;
        return 1;
    }

    server.run();
    return 0;
}
