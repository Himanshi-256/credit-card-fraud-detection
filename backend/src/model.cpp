#include "model.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <unordered_set>

RuleBasedScoringModel::RuleBasedScoringModel()
    : fraudThreshold_(50.0) {}

FraudResult RuleBasedScoringModel::predict(const TransactionInput& input) {
    FraudResult result;
    std::vector<std::string> triggeredReasons;

    // Evaluate individual risk components
    double amountRisk = evaluateAmountRisk(input.amount, triggeredReasons);
    double freqRisk = evaluateFrequencyRisk(input.frequency, triggeredReasons);
    double deviceRisk = evaluateDeviceRisk(input.deviceRecognized, triggeredReasons);
    double locRisk = evaluateLocationRisk(input.location, triggeredReasons);

    // Calculate composite risk score (base line = 5.0)
    double compositeScore = 5.0 + amountRisk + freqRisk + deviceRisk + locRisk;

    // Multi-factor synergy / compounding effect:
    // If an unrecognized device is paired with either high amount or high frequency,
    // the likelihood of automated fraud / account takeover spikes significantly.
    if (!input.deviceRecognized && input.amount > 50000.0) {
        compositeScore += 10.0;
    }
    if (!input.deviceRecognized && input.frequency >= 5) {
        compositeScore += 8.0;
    }

    // Clamp score to [0.0, 100.0]
    compositeScore = std::max(0.0, std::min(100.0, compositeScore));
    // Round to 1 decimal place
    result.riskScore = std::round(compositeScore * 10.0) / 10.0;

    // Classification
    if (result.riskScore >= fraudThreshold_) {
        result.status = "FRAUD";
    } else {
        result.status = "LEGITIMATE";
    }

    result.detailedReasons = triggeredReasons;

    // Format human-readable explanation
    if (triggeredReasons.empty()) {
        result.reason = "Normal transaction pattern";
    } else if (triggeredReasons.size() == 1) {
        result.reason = triggeredReasons[0];
    } else if (triggeredReasons.size() == 2) {
        result.reason = triggeredReasons[0] + " and " + triggeredReasons[1];
    } else {
        std::ostringstream oss;
        for (size_t i = 0; i < triggeredReasons.size(); ++i) {
            oss << triggeredReasons[i];
            if (i < triggeredReasons.size() - 2) {
                oss << ", ";
            } else if (i == triggeredReasons.size() - 2) {
                oss << ", and ";
            }
        }
        result.reason = oss.str();
    }

    return result;
}

std::string RuleBasedScoringModel::getModelName() const {
    return "RuleBased-MultiFactor-ScoringEngine";
}

std::string RuleBasedScoringModel::getModelVersion() const {
    return "1.2.0";
}

void RuleBasedScoringModel::setFraudScoreThreshold(double threshold) {
    fraudThreshold_ = threshold;
}

double RuleBasedScoringModel::getFraudScoreThreshold() const {
    return fraudThreshold_;
}

double RuleBasedScoringModel::evaluateAmountRisk(double amount, std::vector<std::string>& reasons) const {
    if (amount >= 100000.0) {
        reasons.push_back("Extremely high transaction amount");
        return 40.0;
    } else if (amount >= 50000.0) {
        reasons.push_back("High transaction amount");
        return 32.0;
    } else if (amount >= 25000.0) {
        reasons.push_back("Elevated transaction amount");
        return 18.0;
    } else if (amount >= 10000.0) {
        return 8.0;
    }
    return 0.0;
}

double RuleBasedScoringModel::evaluateFrequencyRisk(int frequency, std::vector<std::string>& reasons) const {
    if (frequency >= 10) {
        reasons.push_back("Rapid burst of transactions");
        return 30.0;
    } else if (frequency >= 6) {
        reasons.push_back("Unusually high transaction frequency");
        return 22.0;
    } else if (frequency >= 4) {
        reasons.push_back("Elevated transaction velocity");
        return 14.0;
    } else if (frequency >= 2) {
        return 4.0;
    }
    return 0.0;
}

double RuleBasedScoringModel::evaluateDeviceRisk(bool deviceRecognized, std::vector<std::string>& reasons) const {
    if (!deviceRecognized) {
        reasons.push_back("Unrecognized device");
        return 30.0;
    }
    return 0.0;
}

double RuleBasedScoringModel::evaluateLocationRisk(const std::string& location, std::vector<std::string>& reasons) const {
    // Known high-risk or anomalous test locations
    static const std::unordered_set<std::string> highRiskLocations = {
        "Unknown Location",
        "International Proxy",
        "Offshore Island",
        "Darknet Gateway",
        "Tor Exit Node",
        "Suspicious Foreign IP",
        "High Risk Zone"
    };

    if (highRiskLocations.find(location) != highRiskLocations.end()) {
        reasons.push_back("Suspicious transaction location");
        return 25.0;
    }

    return 0.0;
}
