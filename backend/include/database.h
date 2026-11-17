#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <mutex>
#include "sqlite3.h"

/**
 * @brief Represents a transaction entity stored in the database.
 */
struct TransactionRecord {
    std::string id;               // e.g. "TX2045"
    double amount;                // Transaction amount
    std::string location;         // Location name (e.g. "Delhi")
    int frequency;                // Recent transactions count in last 24h
    bool deviceRecognized;        // true if device is known, false otherwise
    double riskScore;             // Risk score (0.0 to 100.0)
    std::string status;           // "FRAUD" or "LEGITIMATE"
    std::string reason;           // Explanations for the risk score
    std::string timestamp;        // ISO 8601 or YYYY-MM-DD HH:MM:SS
};

/**
 * @brief Represents a generated fraud alert.
 */
struct FraudAlert {
    std::string alertId;          // e.g. "ALT1001"
    std::string transactionId;    // Related transaction ID
    std::string severity;         // "CRITICAL", "HIGH", "MEDIUM"
    std::string message;          // Reason / alert description
    std::string createdAt;        // Timestamp when alert was logged
};

/**
 * @brief Aggregate statistics for the dashboard.
 */
struct DashboardStats {
    int totalTransactions;
    int totalFraud;
    int totalLegitimate;
    double fraudPercentage;
    double averageRiskScore;
};

/**
 * @brief Manages SQLite database operations for transactions and fraud alerts.
 * Uses prepared statements to prevent SQL injection vulnerabilities.
 */
class Database {
public:
    explicit Database(const std::string& dbPath);
    ~Database();

    // Disable copy
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    /**
     * @brief Opens the SQLite database connection and initializes the schema.
     * @return true if successful, false otherwise.
     */
    bool initialize();

    /**
     * @brief Checks if the database is open and reachable.
     */
    bool isConnected() const;

    /**
     * @brief Inserts a new transaction record into the database.
     */
    bool insertTransaction(const TransactionRecord& tx);

    /**
     * @brief Inserts a new fraud alert record.
     */
    bool insertAlert(const FraudAlert& alert);

    /**
     * @brief Retrieves a transaction by its unique ID.
     */
    std::optional<TransactionRecord> getTransactionById(const std::string& id);

    /**
     * @brief Retrieves a list of transactions with optional pagination.
     * @param limit Maximum number of records to return.
     * @param offset Number of records to skip.
     */
    std::vector<TransactionRecord> getAllTransactions(int limit = 100, int offset = 0);

    /**
     * @brief Gets total count of transactions stored.
     */
    int getTotalTransactionCount();

    /**
     * @brief Retrieves all fraud alerts, ordered newest first.
     */
    std::vector<FraudAlert> getAllAlerts(int limit = 50);

    /**
     * @brief Computes aggregate statistics for the dashboard.
     */
    DashboardStats getStats();

    /**
     * @brief Generates a unique transaction identifier (e.g. TX1001).
     */
    std::string generateTransactionId();

    /**
     * @brief Generates a unique alert identifier (e.g. ALT1001).
     */
    std::string generateAlertId();

    /**
     * @brief Seeds initial sample dataset if table is currently empty.
     */
    bool seedFromCsv(const std::string& csvFilePath);

private:
    std::string dbPath_;
    sqlite3* dbHandle_;
    mutable std::mutex dbMutex_; // Thread-safety for SQLite access

    bool createTables();
    static std::string getCurrentUtcTimestamp();
};

#endif // DATABASE_H
