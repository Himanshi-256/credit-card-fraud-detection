#include "server.h"
#include <iostream>

Server::Server(const ServerConfig& config)
    : config_(config) {}

bool Server::init() {
    std::cout << "==================================================" << std::endl;
    std::cout << "  CREDIT CARD FRAUD DETECTION SYSTEM - C++ BACKEND " << std::endl;
    std::cout << "==================================================" << std::endl;

    // 1. Initialize SQLite Database
    std::cout << "[Init] Connecting to SQLite database: " << config_.dbPath << " ..." << std::endl;
    db_ = std::make_shared<Database>(config_.dbPath);
    if (!db_->initialize()) {
        std::cerr << "[Error] Failed to initialize database at " << config_.dbPath << std::endl;
        return false;
    }
    std::cout << "[Init] Database initialized successfully." << std::endl;

    // 2. Initialize Fraud Detection Engine
    std::cout << "[Init] Loading Fraud Detection Engine..." << std::endl;
    detector_ = std::make_shared<FraudDetector>();
    std::cout << "[Init] Active model: " << detector_->getActiveModelInfo() << std::endl;

    // 3. Seed initial dataset if database is empty
    if (config_.seedSampleData) {
        std::cout << "[Init] Checking for dataset seeding from " << config_.csvPath << " ..." << std::endl;
        db_->seedFromCsv(config_.csvPath);
    }

    // 4. Register REST API routes
    std::cout << "[Init] Registering REST API routes..." << std::endl;
    router_ = std::make_unique<ApiRouter>(db_, detector_);
    router_->registerRoutes(app_);
    std::cout << "[Init] REST API routes configured with CORS support." << std::endl;

    return true;
}

void Server::run() {
    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << " REST API Endpoints active:" << std::endl;
    std::cout << "   POST http://localhost:" << config_.port << "/api/transactions" << std::endl;
    std::cout << "   GET  http://localhost:" << config_.port << "/api/transactions" << std::endl;
    std::cout << "   GET  http://localhost:" << config_.port << "/api/transactions/{id}" << std::endl;
    std::cout << "   GET  http://localhost:" << config_.port << "/api/stats" << std::endl;
    std::cout << "   GET  http://localhost:" << config_.port << "/api/alerts" << std::endl;
    std::cout << "   GET  http://localhost:" << config_.port << "/api/health" << std::endl;
    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << "[Server] Listening on http://0.0.0.0:" << config_.port << std::endl;

    app_.loglevel(crow::LogLevel::Warning);
    app_.port(config_.port).multithreaded().run();
}

void Server::stop() {
    std::cout << "[Server] Stopping Crow server..." << std::endl;
    app_.stop();
}
