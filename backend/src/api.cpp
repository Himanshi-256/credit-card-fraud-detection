#include "api.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

ApiRouter::ApiRouter(std::shared_ptr<Database> db, std::shared_ptr<FraudDetector> detector)
    : db_(std::move(db)), detector_(std::move(detector)) {}

void ApiRouter::applyCorsHeaders(crow::response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS, PUT, DELETE");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, Accept, X-Requested-With");
    res.set_header("Access-Control-Max-Age", "86400");
}

crow::response ApiRouter::makeErrorResponse(int statusCode, const std::string& message) {
    nlohmann::json errJson;
    errJson["error"] = message;
    errJson["status"] = statusCode;

    crow::response res(statusCode, errJson.dump());
    res.set_header("Content-Type", "application/json");
    applyCorsHeaders(res);
    return res;
}

crow::response ApiRouter::makeJsonResponse(int statusCode, const nlohmann::json& body) {
    crow::response res(statusCode, body.dump());
    res.set_header("Content-Type", "application/json");
    applyCorsHeaders(res);
    return res;
}

void ApiRouter::registerRoutes(CrowApp& app) {
    // -------------------------------------------------------------
    // Health Check Endpoint
    // GET /api/health
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/health").methods(crow::HTTPMethod::Get)
    ([this]() {
        return handleHealthCheck();
    });

    // -------------------------------------------------------------
    // Create & Evaluate Transaction Endpoint
    // POST /api/transactions
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/transactions").methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        return handlePostTransaction(req);
    });

    // -------------------------------------------------------------
    // Retrieve All Transactions Endpoint
    // GET /api/transactions
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/transactions").methods(crow::HTTPMethod::Get)
    ([this](const crow::request& req) {
        return handleGetAllTransactions(req);
    });

    // -------------------------------------------------------------
    // Retrieve Specific Transaction Endpoint
    // GET /api/transactions/<id>
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/transactions/<string>").methods(crow::HTTPMethod::Get)
    ([this](const std::string& id) {
        return handleGetTransactionById(id);
    });

    // -------------------------------------------------------------
    // Dashboard Aggregate Statistics Endpoint
    // GET /api/stats
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/stats").methods(crow::HTTPMethod::Get)
    ([this]() {
        return handleGetStats();
    });

    // -------------------------------------------------------------
    // Retrieve Fraud Alerts Endpoint
    // GET /api/alerts
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/alerts").methods(crow::HTTPMethod::Get)
    ([this](const crow::request& req) {
        return handleGetAlerts(req);
    });
}

crow::response ApiRouter::handleOptionsPreflight() {
    crow::response res(204);
    applyCorsHeaders(res);
    return res;
}

crow::response ApiRouter::handleHealthCheck() {
    bool dbConnected = db_ && db_->isConnected();
    nlohmann::json res;
    res["status"] = "UP";
    res["database"] = dbConnected ? "CONNECTED" : "DISCONNECTED";
    res["activeModel"] = detector_ ? detector_->getActiveModelInfo() : "Unavailable";
    res["server"] = "Crow C++ REST Server";

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    struct tm tm_buf;
#if defined(_WIN32)
    gmtime_s(&tm_buf, &in_time_t);
#else
    gmtime_r(&in_time_t, &tm_buf);
#endif
    ss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
    res["timestamp"] = ss.str();

    return makeJsonResponse(200, res);
}

crow::response ApiRouter::handlePostTransaction(const crow::request& req) {
    if (!db_ || !db_->isConnected()) {
        return makeErrorResponse(500, "Database service is unavailable.");
    }

    nlohmann::json payload;
    try {
        payload = nlohmann::json::parse(req.body);
    } catch (const std::exception& e) {
        return makeErrorResponse(400, std::string("Malformed JSON body: ") + e.what());
    }

    // Step 1: Validate incoming transaction details
    ValidationResult valResult = FraudDetector::validateJsonInput(payload);
    if (!valResult.isValid) {
        return makeErrorResponse(400, valResult.errorMessage);
    }

    // Step 2: Parse and run through the Fraud Detection Engine
    TransactionInput input = FraudDetector::parseJsonInput(payload);
    FraudResult prediction = detector_->detectFraud(input);

    // Step 3: Generate IDs and prepare database record
    std::string txId = db_->generateTransactionId();
    TransactionRecord record;
    record.id = txId;
    record.amount = input.amount;
    record.location = input.location;
    record.frequency = input.frequency;
    record.deviceRecognized = input.deviceRecognized;
    record.riskScore = prediction.riskScore;
    record.status = prediction.status;
    record.reason = prediction.reason;

    // Step 4: Save transaction record to SQLite
    if (!db_->insertTransaction(record)) {
        return makeErrorResponse(500, "Failed to persist transaction to database.");
    }

    // Step 5: If classified as FRAUD, automatically generate a Fraud Alert
    if (prediction.status == "FRAUD") {
        FraudAlert alert;
        alert.alertId = db_->generateAlertId();
        alert.transactionId = txId;
        alert.severity = FraudDetector::calculateSeverity(prediction.riskScore);
        alert.message = prediction.reason;
        db_->insertAlert(alert);
    }

    // Step 6: Return standard response
    nlohmann::json res;
    res["transactionId"] = txId;
    res["status"] = prediction.status;
    res["riskScore"] = prediction.riskScore;
    res["reason"] = prediction.reason;

    return makeJsonResponse(201, res);
}

crow::response ApiRouter::handleGetAllTransactions(const crow::request& req) {
    if (!db_ || !db_->isConnected()) {
        return makeErrorResponse(500, "Database service is unavailable.");
    }

    int limit = 100;
    int offset = 0;

    if (req.url_params.get("limit") != nullptr) {
        try {
            limit = std::max(1, std::min(500, std::stoi(req.url_params.get("limit"))));
        } catch (...) {}
    }

    if (req.url_params.get("offset") != nullptr) {
        try {
            offset = std::max(0, std::stoi(req.url_params.get("offset")));
        } catch (...) {}
    }

    auto records = db_->getAllTransactions(limit, offset);
    int totalCount = db_->getTotalTransactionCount();

    nlohmann::json txList = nlohmann::json::array();
    for (const auto& r : records) {
        nlohmann::json item;
        item["id"] = r.id;
        item["amount"] = r.amount;
        item["location"] = r.location;
        item["frequency"] = r.frequency;
        item["deviceRecognized"] = r.deviceRecognized;
        item["riskScore"] = r.riskScore;
        item["status"] = r.status;
        item["reason"] = r.reason;
        item["timestamp"] = r.timestamp;
        txList.push_back(item);
    }

    nlohmann::json responseJson;
    responseJson["total"] = totalCount;
    responseJson["limit"] = limit;
    responseJson["offset"] = offset;
    responseJson["transactions"] = txList;

    return makeJsonResponse(200, responseJson);
}

crow::response ApiRouter::handleGetTransactionById(const std::string& id) {
    if (!db_ || !db_->isConnected()) {
        return makeErrorResponse(500, "Database service is unavailable.");
    }

    auto recordOpt = db_->getTransactionById(id);
    if (!recordOpt.has_value()) {
        return makeErrorResponse(404, "Transaction with ID '" + id + "' not found.");
    }

    const auto& r = recordOpt.value();
    nlohmann::json item;
    item["id"] = r.id;
    item["amount"] = r.amount;
    item["location"] = r.location;
    item["frequency"] = r.frequency;
    item["deviceRecognized"] = r.deviceRecognized;
    item["riskScore"] = r.riskScore;
    item["status"] = r.status;
    item["reason"] = r.reason;
    item["timestamp"] = r.timestamp;

    return makeJsonResponse(200, item);
}

crow::response ApiRouter::handleGetStats() {
    if (!db_ || !db_->isConnected()) {
        return makeErrorResponse(500, "Database service is unavailable.");
    }

    DashboardStats stats = db_->getStats();
    nlohmann::json res;
    res["totalTransactions"] = stats.totalTransactions;
    res["totalFraud"] = stats.totalFraud;
    res["totalLegitimate"] = stats.totalLegitimate;
    res["fraudPercentage"] = stats.fraudPercentage;
    res["averageRiskScore"] = stats.averageRiskScore;

    return makeJsonResponse(200, res);
}

crow::response ApiRouter::handleGetAlerts(const crow::request& req) {
    if (!db_ || !db_->isConnected()) {
        return makeErrorResponse(500, "Database service is unavailable.");
    }

    int limit = 50;
    if (req.url_params.get("limit") != nullptr) {
        try {
            limit = std::max(1, std::min(200, std::stoi(req.url_params.get("limit"))));
        } catch (...) {}
    }

    auto alerts = db_->getAllAlerts(limit);
    nlohmann::json alertsList = nlohmann::json::array();
    for (const auto& a : alerts) {
        nlohmann::json item;
        item["alertId"] = a.alertId;
        item["transactionId"] = a.transactionId;
        item["severity"] = a.severity;
        item["message"] = a.message;
        item["createdAt"] = a.createdAt;
        alertsList.push_back(item);
    }

    return makeJsonResponse(200, alertsList);
}
