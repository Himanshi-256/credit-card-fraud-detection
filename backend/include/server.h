#ifndef SERVER_H
#define SERVER_H

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <memory>
#include <string>
#include "crow_all.h"
#include "database.h"
#include "fraud_detector.h"
#include "api.h"

/**
 * @brief Server configuration parameters.
 */
struct ServerConfig {
    int port = 8080;
    std::string dbPath = "database/fraud_detection.db";
    std::string csvPath = "dataset/transactions.csv";
    bool seedSampleData = true;
};

/**
 * @brief Manages server lifecycle, dependency injection, and Crow app loop.
 */
class Server {
public:
    explicit Server(const ServerConfig& config);
    ~Server() = default;

    /**
     * @brief Initializes dependencies (DB, Fraud Engine, API Routes).
     * @return true if initialization succeeded.
     */
    bool init();

    /**
     * @brief Starts the HTTP REST API server (blocking call).
     */
    void run();

    /**
     * @brief Signals server to stop.
     */
    void stop();

    std::shared_ptr<Database> getDatabase() const { return db_; }
    std::shared_ptr<FraudDetector> getDetector() const { return detector_; }

private:
    ServerConfig config_;
    CrowApp app_;
    std::shared_ptr<Database> db_;
    std::shared_ptr<FraudDetector> detector_;
    std::unique_ptr<ApiRouter> router_;
};

#endif // SERVER_H
