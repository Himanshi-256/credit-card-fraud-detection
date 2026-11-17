#ifndef FRAUD_DETECTOR_H
#define FRAUD_DETECTOR_H

#include <string>
#include <memory>
#include "model.h"
#include "nlohmann/json.hpp"

/**
 * @brief Result of request input validation.
 */
struct ValidationResult {
    bool isValid;
    std::string errorMessage;
};

/**
 * @brief Main fraud detection engine class.
 * Orchestrates validation, feature evaluation via IFraudModel,
 * and alert severity classification.
 */
class FraudDetector {
public:
    FraudDetector();
    explicit FraudDetector(std::unique_ptr<IFraudModel> model);

    /**
     * @brief Validates raw JSON payload according to domain rules:
     * - Amount must be positive (> 0)
     * - Location must not be empty
     * - Frequency must not be negative (>= 0)
     * - deviceRecognized must be boolean
     */
    static ValidationResult validateJsonInput(const nlohmann::json& payload);

    /**
     * @brief Parses JSON payload into a strongly typed TransactionInput.
     * Assumes validateJsonInput has already passed.
     */
    static TransactionInput parseJsonInput(const nlohmann::json& payload);

    /**
     * @brief Executes fraud evaluation on the transaction input.
     */
    FraudResult detectFraud(const TransactionInput& input);

    /**
     * @brief Determines alert severity based on calculated risk score:
     * - score >= 80: "CRITICAL"
     * - score >= 65: "HIGH"
     * - score >= 50: "MEDIUM"
     */
    static std::string calculateSeverity(double riskScore);

    /**
     * @brief Swaps the underlying scoring model (e.g. for ML model integration).
     */
    void setModel(std::unique_ptr<IFraudModel> model);

    /**
     * @brief Gets current active model info.
     */
    std::string getActiveModelInfo() const;

private:
    std::unique_ptr<IFraudModel> model_;
};

#endif // FRAUD_DETECTOR_H
