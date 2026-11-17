#ifndef API_H
#define API_H

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <memory>
#include "crow_all.h"
#include "database.h"
#include "fraud_detector.h"
#include "nlohmann/json.hpp"

/**
 * @brief Crow middleware that injects CORS headers on every HTTP response,
 *        including auto-generated OPTIONS (preflight) replies.
 */
struct CorsMiddleware {
    struct context {};

    void before_handle(crow::request& /*req*/, crow::response& /*res*/, context& /*ctx*/) {
        // Nothing to do before the handler runs.
    }

    void after_handle(crow::request& /*req*/, crow::response& res, context& /*ctx*/) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods",
                        "GET, POST, OPTIONS, PUT, DELETE");
        res.set_header("Access-Control-Allow-Headers",
                        "Content-Type, Authorization, Accept, X-Requested-With");
        res.set_header("Access-Control-Max-Age", "86400");
    }
};

/// The Crow application type used throughout the project (with CORS middleware).
using CrowApp = crow::App<CorsMiddleware>;

/**
 * @brief REST API Router and Controller.
 * Registers HTTP endpoints, handles request/response serialization,
 * CORS headers, and error dispatching.
 */
class ApiRouter {
public:
    ApiRouter(std::shared_ptr<Database> db, std::shared_ptr<FraudDetector> detector);

    /**
     * @brief Registers all /api/* routes on the Crow application.
     */
    void registerRoutes(CrowApp& app);

    /**
     * @brief Helper to attach standard CORS headers to a Crow response.
     */
    static void applyCorsHeaders(crow::response& res);

    /**
     * @brief Creates a standardized JSON error response.
     */
    static crow::response makeErrorResponse(int statusCode, const std::string& message);

    /**
     * @brief Creates a standardized JSON success response.
     */
    static crow::response makeJsonResponse(int statusCode, const nlohmann::json& body);

private:
    std::shared_ptr<Database> db_;
    std::shared_ptr<FraudDetector> detector_;

    // Route handlers
    crow::response handlePostTransaction(const crow::request& req);
    crow::response handleGetAllTransactions(const crow::request& req);
    crow::response handleGetTransactionById(const std::string& id);
    crow::response handleGetStats();
    crow::response handleGetAlerts(const crow::request& req);
    crow::response handleHealthCheck();
    crow::response handleOptionsPreflight();
};

#endif // API_H
