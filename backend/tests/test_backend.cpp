#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include "database.h"
#include "fraud_detector.h"
#include "model.h"
#include "nlohmann/json.hpp"

#define TEST_PASS(desc) std::cout << "  [PASS] " << desc << std::endl
#define TEST_FAIL(desc) do { std::cerr << "  [FAIL] " << desc << " (Line " << __LINE__ << ")" << std::endl; exit(1); } while(0)
#define ASSERT_TRUE(cond, desc) do { if (cond) { TEST_PASS(desc); } else { TEST_FAIL(desc); } } while(0)

int main() {
    std::cout << "\n========================================================" << std::endl;
    std::cout << " RUNNING UNIT & INTEGRATION TESTS FOR FRAUD DETECTION" << std::endl;
    std::cout << "========================================================\n" << std::endl;

    FraudDetector detector;

    // ---------------------------------------------------------
    // Scenario 1: A normal transaction
    // ---------------------------------------------------------
    std::cout << "[Scenario 1] Testing normal legitimate transaction..." << std::endl;
    {
        TransactionInput normalInput = {1200.0, "Delhi", 1, true};
        FraudResult res = detector.detectFraud(normalInput);

        std::cout << "    Input: Amount=1200, Location='Delhi', Frequency=1, DeviceRecognized=true\n"
                  << "    Output: Status=" << res.status << ", RiskScore=" << res.riskScore
                  << ", Reason='" << res.reason << "'" << std::endl;

        ASSERT_TRUE(res.status == "LEGITIMATE", "Normal transaction is classified as LEGITIMATE");
        ASSERT_TRUE(res.riskScore < 50.0, "Risk score is below 50 threshold");
    }

    // ---------------------------------------------------------
    // Scenario 2: Transaction with an unusually high amount
    // ---------------------------------------------------------
    std::cout << "\n[Scenario 2] Testing transaction with unusually high amount..." << std::endl;
    {
        TransactionInput highAmountInput = {150000.0, "Mumbai", 1, true};
        FraudResult res = detector.detectFraud(highAmountInput);

        std::cout << "    Input: Amount=150000, Location='Mumbai', Frequency=1, DeviceRecognized=true\n"
                  << "    Output: Status=" << res.status << ", RiskScore=" << res.riskScore
                  << ", Reason='" << res.reason << "'" << std::endl;

        ASSERT_TRUE(res.riskScore >= 45.0, "High amount significantly elevates risk score");
        ASSERT_TRUE(res.reason.find("high transaction amount") != std::string::npos ||
                    res.reason.find("Extremely high") != std::string::npos,
                    "Explanation specifically highlights high transaction amount");
    }

    // ---------------------------------------------------------
    // Scenario 3: Transaction from an unrecognized device
    // ---------------------------------------------------------
    std::cout << "\n[Scenario 3] Testing transaction from unrecognized device..." << std::endl;
    {
        TransactionInput unrecDeviceInput = {60000.0, "Delhi", 2, false};
        FraudResult res = detector.detectFraud(unrecDeviceInput);

        std::cout << "    Input: Amount=60000, Location='Delhi', Frequency=2, DeviceRecognized=false\n"
                  << "    Output: Status=" << res.status << ", RiskScore=" << res.riskScore
                  << ", Reason='" << res.reason << "'" << std::endl;

        ASSERT_TRUE(res.status == "FRAUD", "High amount from unrecognized device is flagged as FRAUD");
        ASSERT_TRUE(res.reason.find("Unrecognized device") != std::string::npos,
                    "Explanation notes unrecognized device");
    }

    // ---------------------------------------------------------
    // Scenario 4: Transaction with unusually high frequency
    // ---------------------------------------------------------
    std::cout << "\n[Scenario 4] Testing transaction with unusually high frequency..." << std::endl;
    {
        TransactionInput highFreqInput = {45000.0, "Bengaluru", 9, false};
        FraudResult res = detector.detectFraud(highFreqInput);

        std::cout << "    Input: Amount=45000, Location='Bengaluru', Frequency=9, DeviceRecognized=false\n"
                  << "    Output: Status=" << res.status << ", RiskScore=" << res.riskScore
                  << ", Reason='" << res.reason << "'" << std::endl;

        ASSERT_TRUE(res.status == "FRAUD", "High velocity transaction burst flagged as FRAUD");
        ASSERT_TRUE(res.riskScore >= 70.0, "Score reflects critical risk");
    }

    // ---------------------------------------------------------
    // Scenario 5: Invalid transaction requests
    // ---------------------------------------------------------
    std::cout << "\n[Scenario 5] Testing invalid transaction validation rules..." << std::endl;
    {
        // Negative amount
        nlohmann::json badAmount = {{"amount", -500}, {"location", "Delhi"}, {"frequency", 1}, {"deviceRecognized", true}};
        ValidationResult val1 = FraudDetector::validateJsonInput(badAmount);
        ASSERT_TRUE(!val1.isValid, "Negative amount is rejected");

        // Zero amount
        nlohmann::json zeroAmount = {{"amount", 0}, {"location", "Delhi"}, {"frequency", 1}, {"deviceRecognized", true}};
        ValidationResult val2 = FraudDetector::validateJsonInput(zeroAmount);
        ASSERT_TRUE(!val2.isValid, "Zero amount is rejected");

        // Empty location
        nlohmann::json emptyLoc = {{"amount", 500}, {"location", "   "}, {"frequency", 1}, {"deviceRecognized", true}};
        ValidationResult val3 = FraudDetector::validateJsonInput(emptyLoc);
        ASSERT_TRUE(!val3.isValid, "Empty whitespace location is rejected");

        // Negative frequency
        nlohmann::json negFreq = {{"amount", 500}, {"location", "Delhi"}, {"frequency", -2}, {"deviceRecognized", true}};
        ValidationResult val4 = FraudDetector::validateJsonInput(negFreq);
        ASSERT_TRUE(!val4.isValid, "Negative frequency is rejected");

        // Non-boolean deviceRecognized
        nlohmann::json badDevice = {{"amount", 500}, {"location", "Delhi"}, {"frequency", 1}, {"deviceRecognized", "yes"}};
        ValidationResult val5 = FraudDetector::validateJsonInput(badDevice);
        ASSERT_TRUE(!val5.isValid, "Non-boolean deviceRecognized is rejected");

        // Valid payload
        nlohmann::json validPayload = {{"amount", 80000}, {"location", "Delhi"}, {"frequency", 7}, {"deviceRecognized", false}};
        ValidationResult val6 = FraudDetector::validateJsonInput(validPayload);
        ASSERT_TRUE(val6.isValid, "Valid payload passes all checks");
    }

    // ---------------------------------------------------------
    // Scenario 6: Database Persistence & History Retrieval
    // ---------------------------------------------------------
    std::cout << "\n[Scenario 6] Testing SQLite persistence and transaction retrieval..." << std::endl;
    {
        Database testDb(":memory:");
        ASSERT_TRUE(testDb.initialize(), "In-memory test database initialized");

        TransactionRecord tx1;
        tx1.id = "TX2045";
        tx1.amount = 80000.0;
        tx1.location = "Delhi";
        tx1.frequency = 7;
        tx1.deviceRecognized = false;
        tx1.riskScore = 92.0;
        tx1.status = "FRAUD";
        tx1.reason = "High transaction amount and unrecognized device";
        tx1.timestamp = "2026-11-16 23:45:00";

        ASSERT_TRUE(testDb.insertTransaction(tx1), "Inserted TX2045 into database");

        auto retrieved = testDb.getTransactionById("TX2045");
        ASSERT_TRUE(retrieved.has_value(), "Retrieved TX2045 by ID");
        ASSERT_TRUE(retrieved->amount == 80000.0, "Transaction amount matches stored value");
        ASSERT_TRUE(retrieved->status == "FRAUD", "Transaction status matches stored value");
        ASSERT_TRUE(retrieved->riskScore == 92.0, "Transaction risk score matches stored value");

        auto notFound = testDb.getTransactionById("NON_EXISTENT");
        ASSERT_TRUE(!notFound.has_value(), "Non-existent transaction returns nullopt");

        auto allList = testDb.getAllTransactions();
        ASSERT_TRUE(allList.size() == 1, "getAllTransactions returned 1 record");
    }

    // ---------------------------------------------------------
    // Scenario 7: Dashboard Statistics Calculation
    // ---------------------------------------------------------
    std::cout << "\n[Scenario 7] Testing dashboard statistics computation..." << std::endl;
    {
        Database testDb(":memory:");
        testDb.initialize();

        // Insert 1 legitimate, 1 fraud
        TransactionRecord txLegit = {"TX1001", 1000.0, "Delhi", 1, true, 10.0, "LEGITIMATE", "Normal", "2026-11-16"};
        TransactionRecord txFraud = {"TX1002", 90000.0, "Delhi", 6, false, 90.0, "FRAUD", "High risk", "2026-11-16"};

        testDb.insertTransaction(txLegit);
        testDb.insertTransaction(txFraud);

        DashboardStats stats = testDb.getStats();
        ASSERT_TRUE(stats.totalTransactions == 2, "Total transactions is 2");
        ASSERT_TRUE(stats.totalLegitimate == 1, "Total legitimate is 1");
        ASSERT_TRUE(stats.totalFraud == 1, "Total fraud is 1");
        ASSERT_TRUE(stats.fraudPercentage == 50.0, "Fraud percentage is 50.0%");
        ASSERT_TRUE(stats.averageRiskScore == 50.0, "Average risk score is 50.0");
    }

    // ---------------------------------------------------------
    // Scenario 8: Fraud Alerts Generation & Retrieval
    // ---------------------------------------------------------
    std::cout << "\n[Scenario 8] Testing fraud alert logging and severity mapping..." << std::endl;
    {
        Database testDb(":memory:");
        testDb.initialize();

        TransactionRecord txFraud = {"TX2045", 80000.0, "Delhi", 7, false, 92.0, "FRAUD", "High amount", "2026-11-16"};
        testDb.insertTransaction(txFraud);

        FraudAlert alert;
        alert.alertId = "ALT1001";
        alert.transactionId = "TX2045";
        alert.severity = FraudDetector::calculateSeverity(txFraud.riskScore);
        alert.message = "High transaction amount and unrecognized device";
        alert.createdAt = "2026-11-16 23:45:00";

        ASSERT_TRUE(alert.severity == "CRITICAL", "Score 92 maps to CRITICAL severity");
        ASSERT_TRUE(testDb.insertAlert(alert), "Fraud alert inserted into database");

        auto alerts = testDb.getAllAlerts();
        ASSERT_TRUE(alerts.size() == 1, "Retrieved 1 fraud alert");
        ASSERT_TRUE(alerts[0].transactionId == "TX2045", "Alert references related transaction TX2045");
        ASSERT_TRUE(alerts[0].severity == "CRITICAL", "Alert severity preserved");
    }

    std::cout << "\n========================================================" << std::endl;
    std::cout << " ALL 8 TEST SUITE SCENARIOS PASSED SUCCESSFULLY! [100%]" << std::endl;
    std::cout << "========================================================\n" << std::endl;

    return 0;
}
