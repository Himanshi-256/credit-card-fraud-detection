#include "fraud_detector.h"
#include <iostream>

FraudDetector::FraudDetector()
    : model_(std::make_unique<RuleBasedScoringModel>()) {}

FraudDetector::FraudDetector(std::unique_ptr<IFraudModel> model)
    : model_(std::move(model)) {}

ValidationResult FraudDetector::validateJsonInput(const nlohmann::json& payload) {
    if (!payload.is_object()) {
        return {false, "Request payload must be a JSON object."};
    }

    // 1. Amount validation
    if (!payload.contains("amount")) {
        return {false, "Missing required field: 'amount'."};
    }
    if (!payload["amount"].is_number()) {
        return {false, "Field 'amount' must be a numeric value."};
    }
    double amount = payload["amount"].get<double>();
    if (amount <= 0.0) {
        return {false, "Validation failed: Amount must be positive (greater than 0)."};
    }

    // 2. Location validation
    if (!payload.contains("location")) {
        return {false, "Missing required field: 'location'."};
    }
    if (!payload["location"].is_string()) {
        return {false, "Field 'location' must be a string."};
    }
    std::string location = payload["location"].get<std::string>();
    // Trim whitespace
    size_t first = location.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {false, "Validation failed: Location must not be empty."};
    }

    // 3. Frequency validation
    if (!payload.contains("frequency")) {
        return {false, "Missing required field: 'frequency'."};
    }
    if (!payload["frequency"].is_number_integer()) {
        return {false, "Field 'frequency' must be an integer."};
    }
    int frequency = payload["frequency"].get<int>();
    if (frequency < 0) {
        return {false, "Validation failed: Frequency must not be negative."};
    }

    // 4. deviceRecognized validation
    if (!payload.contains("deviceRecognized")) {
        return {false, "Missing required field: 'deviceRecognized'."};
    }
    if (!payload["deviceRecognized"].is_boolean()) {
        return {false, "Validation failed: Field 'deviceRecognized' must be a boolean (true or false)."};
    }

    return {true, ""};
}

TransactionInput FraudDetector::parseJsonInput(const nlohmann::json& payload) {
    TransactionInput input;
    input.amount = payload["amount"].get<double>();
    input.location = payload["location"].get<std::string>();
    input.frequency = payload["frequency"].get<int>();
    input.deviceRecognized = payload["deviceRecognized"].get<bool>();
    return input;
}

FraudResult FraudDetector::detectFraud(const TransactionInput& input) {
    if (!model_) {
        // Fallback default if model unset
        model_ = std::make_unique<RuleBasedScoringModel>();
    }
    return model_->predict(input);
}

std::string FraudDetector::calculateSeverity(double riskScore) {
    if (riskScore >= 80.0) {
        return "CRITICAL";
    } else if (riskScore >= 65.0) {
        return "HIGH";
    } else if (riskScore >= 50.0) {
        return "MEDIUM";
    }
    return "LOW";
}

void FraudDetector::setModel(std::unique_ptr<IFraudModel> model) {
    model_ = std::move(model);
}

std::string FraudDetector::getActiveModelInfo() const {
    if (!model_) return "None";
    return model_->getModelName() + " (v" + model_->getModelVersion() + ")";
}
