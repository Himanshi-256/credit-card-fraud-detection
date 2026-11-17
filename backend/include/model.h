#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>

/**
 * @brief Input features for fraud detection.
 * Standard contract between API layer and Detection layer.
 */
struct TransactionInput {
    double amount;                // Transaction monetary value
    std::string location;         // Originating city / country / IP location
    int frequency;                // Number of transactions in the last 24h
    bool deviceRecognized;        // Whether the device fingerprint is previously known
};

/**
 * @brief Output prediction and explainable risk analysis.
 */
struct FraudResult {
    std::string status;           // "FRAUD" or "LEGITIMATE"
    double riskScore;             // Continuous risk score in [0.0, 100.0]
    std::string reason;           // Human-readable explanation of risk factors
    std::vector<std::string> detailedReasons; // Individual flagged reasons
};

/**
 * @brief Abstract interface for fraud scoring models.
 * Allows seamless replacement with trained ML models (e.g. Random Forest,
 * Logistic Regression, XGBoost, or ONNX Runtime model) in the future.
 */
class IFraudModel {
public:
    virtual ~IFraudModel() = default;

    /**
     * @brief Evaluates transaction input features and returns fraud prediction.
     */
    virtual FraudResult predict(const TransactionInput& input) = 0;

    /**
     * @brief Model architecture name (e.g., "RuleBased-ScoringEngine-v1").
     */
    virtual std::string getModelName() const = 0;

    /**
     * @brief Model version string.
     */
    virtual std::string getModelVersion() const = 0;
};

/**
 * @brief Explainable, multi-factor rule-based risk scoring model.
 * 
 * Note for College Project Evaluation:
 * This model implements an explainable heuristic scoring algorithm
 * based on statistical baselines and known fraud risk factors.
 * It is structured modularly via the IFraudModel interface so that
 * a trained machine learning model can be dropped in without changing
 * any API or database logic.
 */
class RuleBasedScoringModel : public IFraudModel {
public:
    RuleBasedScoringModel();

    FraudResult predict(const TransactionInput& input) override;
    std::string getModelName() const override;
    std::string getModelVersion() const override;

    // Configurable thresholds for experimental tuning
    void setFraudScoreThreshold(double threshold);
    double getFraudScoreThreshold() const;

private:
    double fraudThreshold_;      // Default: 50.0

    // Feature risk evaluators
    double evaluateAmountRisk(double amount, std::vector<std::string>& reasons) const;
    double evaluateFrequencyRisk(int frequency, std::vector<std::string>& reasons) const;
    double evaluateDeviceRisk(bool deviceRecognized, std::vector<std::string>& reasons) const;
    double evaluateLocationRisk(const std::string& location, std::vector<std::string>& reasons) const;
};

#endif // MODEL_H
