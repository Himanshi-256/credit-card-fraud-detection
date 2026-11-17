#include "database.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <cmath>

Database::Database(const std::string& dbPath)
    : dbPath_(dbPath), dbHandle_(nullptr) {}

Database::~Database() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (dbHandle_) {
        sqlite3_close(dbHandle_);
        dbHandle_ = nullptr;
    }
}

bool Database::initialize() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    int rc = sqlite3_open(dbPath_.c_str(), &dbHandle_);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database Error] Could not open database at " << dbPath_
                  << ": " << sqlite3_errmsg(dbHandle_) << std::endl;
        return false;
    }

    // Enable foreign keys
    char* errMsg = nullptr;
    rc = sqlite3_exec(dbHandle_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database Warning] Failed to enable foreign keys: "
                  << (errMsg ? errMsg : "Unknown error") << std::endl;
        sqlite3_free(errMsg);
    }

    return createTables();
}

bool Database::isConnected() const {
    std::lock_guard<std::mutex> lock(dbMutex_);
    return dbHandle_ != nullptr;
}

bool Database::createTables() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS transactions ("
        "  id TEXT PRIMARY KEY,"
        "  amount REAL NOT NULL,"
        "  location TEXT NOT NULL,"
        "  frequency INTEGER NOT NULL,"
        "  device INTEGER NOT NULL,"
        "  risk_score REAL NOT NULL,"
        "  status TEXT NOT NULL,"
        "  reason TEXT NOT NULL,"
        "  timestamp TEXT NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS fraud_alerts ("
        "  alert_id TEXT PRIMARY KEY,"
        "  transaction_id TEXT NOT NULL,"
        "  severity TEXT NOT NULL,"
        "  message TEXT NOT NULL,"
        "  created_at TEXT NOT NULL,"
        "  FOREIGN KEY (transaction_id) REFERENCES transactions(id) ON DELETE CASCADE"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_transactions_status ON transactions(status);"
        "CREATE INDEX IF NOT EXISTS idx_alerts_txid ON fraud_alerts(transaction_id);";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(dbHandle_, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database Error] Failed to create tables: "
                  << (errMsg ? errMsg : "Unknown error") << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

bool Database::insertTransaction(const TransactionRecord& tx) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!dbHandle_) return false;

    const char* sql =
        "INSERT INTO transactions (id, amount, location, frequency, device, risk_score, status, reason, timestamp) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(dbHandle_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database Error] Insert transaction prepare failed: "
                  << sqlite3_errmsg(dbHandle_) << std::endl;
        return false;
    }

    std::string ts = tx.timestamp.empty() ? getCurrentUtcTimestamp() : tx.timestamp;

    sqlite3_bind_text(stmt, 1, tx.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, tx.amount);
    sqlite3_bind_text(stmt, 3, tx.location.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, tx.frequency);
    sqlite3_bind_int(stmt, 5, tx.deviceRecognized ? 1 : 0);
    sqlite3_bind_double(stmt, 6, tx.riskScore);
    sqlite3_bind_text(stmt, 7, tx.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, tx.reason.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, ts.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "[Database Error] Insert transaction failed: "
                  << sqlite3_errmsg(dbHandle_) << std::endl;
        return false;
    }

    return true;
}

bool Database::insertAlert(const FraudAlert& alert) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!dbHandle_) return false;

    const char* sql =
        "INSERT INTO fraud_alerts (alert_id, transaction_id, severity, message, created_at) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(dbHandle_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database Error] Insert alert prepare failed: "
                  << sqlite3_errmsg(dbHandle_) << std::endl;
        return false;
    }

    std::string ts = alert.createdAt.empty() ? getCurrentUtcTimestamp() : alert.createdAt;

    sqlite3_bind_text(stmt, 1, alert.alertId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, alert.transactionId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, alert.severity.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, alert.message.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, ts.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "[Database Error] Insert alert failed: "
                  << sqlite3_errmsg(dbHandle_) << std::endl;
        return false;
    }

    return true;
}

std::optional<TransactionRecord> Database::getTransactionById(const std::string& id) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!dbHandle_) return std::nullopt;

    const char* sql =
        "SELECT id, amount, location, frequency, device, risk_score, status, reason, timestamp "
        "FROM transactions WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(dbHandle_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return std::nullopt;

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        TransactionRecord record;
        record.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        record.amount = sqlite3_column_double(stmt, 1);
        record.location = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.frequency = sqlite3_column_int(stmt, 3);
        record.deviceRecognized = (sqlite3_column_int(stmt, 4) != 0);
        record.riskScore = sqlite3_column_double(stmt, 5);
        record.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.reason = sqlite3_column_text(stmt, 7) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)) : "";
        record.timestamp = sqlite3_column_text(stmt, 8) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)) : "";

        sqlite3_finalize(stmt);
        return record;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<TransactionRecord> Database::getAllTransactions(int limit, int offset) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<TransactionRecord> results;
    if (!dbHandle_) return results;

    const char* sql =
        "SELECT id, amount, location, frequency, device, risk_score, status, reason, timestamp "
        "FROM transactions ORDER BY rowid DESC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(dbHandle_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return results;

    sqlite3_bind_int(stmt, 1, limit);
    sqlite3_bind_int(stmt, 2, offset);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TransactionRecord record;
        record.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        record.amount = sqlite3_column_double(stmt, 1);
        record.location = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.frequency = sqlite3_column_int(stmt, 3);
        record.deviceRecognized = (sqlite3_column_int(stmt, 4) != 0);
        record.riskScore = sqlite3_column_double(stmt, 5);
        record.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.reason = sqlite3_column_text(stmt, 7) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)) : "";
        record.timestamp = sqlite3_column_text(stmt, 8) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)) : "";
        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

int Database::getTotalTransactionCount() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!dbHandle_) return 0;

    const char* sql = "SELECT COUNT(*) FROM transactions;";
    sqlite3_stmt* stmt = nullptr;
    int count = 0;
    if (sqlite3_prepare_v2(dbHandle_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return count;
}

std::vector<FraudAlert> Database::getAllAlerts(int limit) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::vector<FraudAlert> results;
    if (!dbHandle_) return results;

    const char* sql =
        "SELECT alert_id, transaction_id, severity, message, created_at "
        "FROM fraud_alerts ORDER BY rowid DESC LIMIT ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(dbHandle_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return results;

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FraudAlert alert;
        alert.alertId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        alert.transactionId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        alert.severity = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        alert.message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        alert.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        results.push_back(alert);
    }

    sqlite3_finalize(stmt);
    return results;
}

DashboardStats Database::getStats() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    DashboardStats stats = {0, 0, 0, 0.0, 0.0};
    if (!dbHandle_) return stats;

    const char* sql =
        "SELECT "
        "  COUNT(*), "
        "  COALESCE(SUM(CASE WHEN status = 'FRAUD' THEN 1 ELSE 0 END), 0), "
        "  COALESCE(SUM(CASE WHEN status = 'LEGITIMATE' THEN 1 ELSE 0 END), 0), "
        "  COALESCE(AVG(risk_score), 0.0) "
        "FROM transactions;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(dbHandle_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.totalTransactions = sqlite3_column_int(stmt, 0);
            stats.totalFraud = sqlite3_column_int(stmt, 1);
            stats.totalLegitimate = sqlite3_column_int(stmt, 2);
            stats.averageRiskScore = sqlite3_column_double(stmt, 3);

            if (stats.totalTransactions > 0) {
                stats.fraudPercentage = (static_cast<double>(stats.totalFraud) * 100.0) / stats.totalTransactions;
                // Round to 2 decimal places
                stats.fraudPercentage = std::round(stats.fraudPercentage * 100.0) / 100.0;
                stats.averageRiskScore = std::round(stats.averageRiskScore * 100.0) / 100.0;
            }
        }
        sqlite3_finalize(stmt);
    }

    return stats;
}

std::string Database::generateTransactionId() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!dbHandle_) return "TX1001";

    const char* sql = "SELECT COUNT(*) FROM transactions;";
    sqlite3_stmt* stmt = nullptr;
    int count = 0;
    if (sqlite3_prepare_v2(dbHandle_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    std::ostringstream oss;
    oss << "TX" << (1001 + count);
    return oss.str();
}

std::string Database::generateAlertId() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!dbHandle_) return "ALT1001";

    const char* sql = "SELECT COUNT(*) FROM fraud_alerts;";
    sqlite3_stmt* stmt = nullptr;
    int count = 0;
    if (sqlite3_prepare_v2(dbHandle_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    std::ostringstream oss;
    oss << "ALT" << (1001 + count);
    return oss.str();
}

std::string Database::getCurrentUtcTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    struct tm tm_buf;
#if defined(_WIN32)
    gmtime_s(&tm_buf, &in_time_t);
#else
    gmtime_r(&in_time_t, &tm_buf);
#endif
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

bool Database::seedFromCsv(const std::string& csvFilePath) {
    if (getTotalTransactionCount() > 0) {
        // Table already populated, no need to re-seed
        return true;
    }

    std::ifstream file(csvFilePath);
    if (!file.is_open()) {
        std::cerr << "[Database Info] No initial CSV dataset found at " << csvFilePath
                  << " (skipping automatic seeding)." << std::endl;
        return false;
    }

    std::string line;
    // Skip header line
    if (!std::getline(file, line)) {
        return false;
    }

    int seededCount = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        // Quote-aware CSV parsing
        std::vector<std::string> tokens;
        std::string current;
        bool inQuotes = false;
        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (c == '"') {
                inQuotes = !inQuotes;
            } else if (c == ',' && !inQuotes) {
                tokens.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        }
        tokens.push_back(current);

        // Trim whitespace and outer quotes
        for (auto& t : tokens) {
            size_t first = t.find_first_not_of(" \r\n\t");
            size_t last = t.find_last_not_of(" \r\n\t");
            if (first != std::string::npos && last != std::string::npos) {
                t = t.substr(first, last - first + 1);
            } else {
                t.clear();
            }
            if (t.size() >= 2 && t.front() == '"' && t.back() == '"') {
                t = t.substr(1, t.size() - 2);
            }
        }

        // Expected format:
        // id, amount, location, frequency, device, risk_score, status, reason, timestamp
        if (tokens.size() >= 8) {
            TransactionRecord tx;
            tx.id = tokens[0];
            tx.amount = std::stod(tokens[1]);
            tx.location = tokens[2];
            tx.frequency = std::stoi(tokens[3]);
            tx.deviceRecognized = (tokens[4] == "1" || tokens[4] == "true" || tokens[4] == "TRUE");
            tx.riskScore = std::stod(tokens[5]);
            tx.status = tokens[6];
            tx.reason = tokens[7];
            tx.timestamp = (tokens.size() >= 9 && !tokens[8].empty()) ? tokens[8] : getCurrentUtcTimestamp();

            insertTransaction(tx);
            seededCount++;

            if (tx.status == "FRAUD") {
                FraudAlert alert;
                alert.alertId = "ALT" + tx.id.substr(2);
                alert.transactionId = tx.id;
                alert.severity = (tx.riskScore >= 80.0) ? "CRITICAL" :
                                 (tx.riskScore >= 65.0) ? "HIGH" : "MEDIUM";
                alert.message = tx.reason;
                alert.createdAt = tx.timestamp;
                insertAlert(alert);
            }
        }
    }

    std::cout << "[Database] Successfully seeded " << seededCount << " transactions from " << csvFilePath << std::endl;
    return true;
}
