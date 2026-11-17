# Credit Card Fraud Detection System

A high-performance **Credit Card Fraud Detection Backend and Dashboard** built with **C++17**, the **Crow REST API Framework**, and **SQLite**.

Developed as an academic/college engineering project following modular software architecture principles.

---

## 1. Technology Stack

- **Programming Language**: C++17
- **Backend Web Framework**: [Crow C++](https://crowcpp.org/) (High-performance C++ REST framework)
- **Database**: SQLite3 with Prepared SQL Statements (Thread-safe, anti-SQL injection)
- **JSON Serialization**: [nlohmann/json](https://github.com/nlohmann/json)
- **Build System**: CMake (version 3.15+) with Ninja / MinGW
- **Frontend Integration**: JavaScript Fetch API, HTML5, CSS3

---

## 2. System Architecture & Folder Structure

```
credit-card-fraud-detection/
│
├── index.html                  # Frontend Dashboard (HTML5)
├── css/
│   └── style.css               # Modern cybersecurity UI styling
├── js/
│   └── script.js               # Frontend JavaScript Fetch API integration
├── assets/                     # Icons and static resources
│
├── backend/
│   ├── include/                # Header declarations
│   │   ├── database.h          # SQLite database interface & models
│   │   ├── api.h               # Crow REST API router & CORS handlers
│   │   ├── fraud_detector.h    # High-level fraud evaluation & validation
│   │   ├── model.h             # Explainable risk-scoring model interface
│   │   └── server.h            # Server lifecycle & dependency wiring
│   │
│   ├── src/                    # Source implementations
│   │   ├── main.cpp            # Server CLI entrypoint & flag parsing
│   │   ├── server.cpp          # Server orchestration & Crow event loop
│   │   ├── database.cpp        # SQLite CRUD & prepared statements
│   │   ├── api.cpp             # HTTP endpoint request/response handlers
│   │   ├── fraud_detector.cpp  # Input validation & severity classification
│   │   └── model.cpp           # Multi-factor explainable risk engine
│   │
│   ├── dataset/
│   │   └── transactions.csv    # Benchmark transactions dataset for initial seeding
│   │
│   ├── database/
│   │   └── fraud_detection.db  # Generated SQLite database file
│   │
│   ├── tests/
│   │   └── test_backend.cpp    # 8-scenario automated unit & integration test suite
│   │
│   ├── third_party/            # Self-contained header-only C++ dependencies
│   │   ├── crow/               # crow_all.h
│   │   ├── asio/               # Standalone Asio headers
│   │   ├── nlohmann/           # json.hpp
│   │   └── sqlite/             # sqlite3.h and sqlite3.c
│   │
│   └── CMakeLists.txt          # Backend CMake build specification
│
├── CMakeLists.txt              # Root build forwarder
├── README.md                   # Complete documentation & usage guide
└── .gitignore
```

---

## 3. Fraud Detection Engine

The fraud detection module calculates a continuous risk score between **0 and 100** by evaluating multiple risk factors:

1. **Transaction Amount**: Flags unusually high transaction amounts (> ₹25,000, > ₹50,000, > ₹100,000).
2. **Transaction Velocity (Frequency)**: Analyzes burst charges within short windows (Frequency > 3, > 6, > 10).
3. **Device Recognition**: Penalizes untrusted / previously unseen devices (+30 risk points).
4. **Location Risk**: Detects suspicious or proxy/darknet originating locations.
5. **Synergy Compounding**: Accounts for combined attack vectors (e.g., Unrecognized device + High transaction amount).

### Classification Thresholds:
- **Risk Score ≥ 50**: Classified as `FRAUD`
  - Score 80–100: `CRITICAL` severity alert
  - Score 65–79: `HIGH` severity alert
  - Score 50–64: `MEDIUM` severity alert
- **Risk Score < 50**: Classified as `LEGITIMATE`

### Pluggable Architecture:
The detection engine implements an abstract `IFraudModel` interface. While currently running an explainable rule-based scoring engine with statistical calibration, it can be replaced with a trained Machine Learning model (e.g. Random Forest, Logistic Regression, or ONNX Runtime) without changing any database or API code.

---

## 4. REST API Documentation

### 4.1 Health Check
- **Endpoint**: `GET /api/health`
- **Response**:
```json
{
  "activeModel": "RuleBased-MultiFactor-ScoringEngine (v1.2.0)",
  "database": "CONNECTED",
  "server": "Crow C++ REST Server",
  "status": "UP",
  "timestamp": "2026-11-17T00:25:00Z"
}
```

### 4.2 Submit & Evaluate Transaction
- **Endpoint**: `POST /api/transactions`
- **Request Body**:
```json
{
  "amount": 80000,
  "location": "Delhi",
  "frequency": 7,
  "deviceRecognized": false
}
```
- **Response (201 Created)**:
```json
{
  "transactionId": "TX1021",
  "status": "FRAUD",
  "riskScore": 100.0,
  "reason": "High transaction amount, Unusually high transaction frequency, and Unrecognized device"
}
```

### 4.3 Validation Error Response
- **Response (400 Bad Request)**:
```json
{
  "error": "Validation failed: Amount must be positive (greater than 0).",
  "status": 400
}
```

### 4.4 Retrieve All Transactions
- **Endpoint**: `GET /api/transactions?limit=50&offset=0`
- **Response (200 OK)**:
```json
{
  "limit": 50,
  "offset": 0,
  "total": 22,
  "transactions": [
    {
      "id": "TX1021",
      "amount": 80000.0,
      "location": "Delhi",
      "frequency": 7,
      "deviceRecognized": false,
      "riskScore": 100.0,
      "status": "FRAUD",
      "reason": "High transaction amount, Unusually high transaction frequency, and Unrecognized device",
      "timestamp": "2026-11-16 18:55:32"
    }
  ]
}
```

### 4.5 Retrieve Transaction by ID
- **Endpoint**: `GET /api/transactions/{id}`
- **Response (200 OK)**:
```json
{
  "id": "TX1021",
  "amount": 80000.0,
  "location": "Delhi",
  "frequency": 7,
  "deviceRecognized": false,
  "riskScore": 100.0,
  "status": "FRAUD",
  "reason": "High transaction amount, Unusually high transaction frequency, and Unrecognized device",
  "timestamp": "2026-11-16 18:55:32"
}
```

### 4.6 Dashboard Aggregate Statistics
- **Endpoint**: `GET /api/stats`
- **Response (200 OK)**:
```json
{
  "totalTransactions": 22,
  "totalFraud": 8,
  "totalLegitimate": 14,
  "fraudPercentage": 36.36,
  "averageRiskScore": 43.45
}
```

### 4.7 Retrieve Fraud Alerts
- **Endpoint**: `GET /api/alerts?limit=20`
- **Response (200 OK)**:
```json
[
  {
    "alertId": "ALT1019",
    "transactionId": "TX1019",
    "severity": "CRITICAL",
    "message": "High transaction amount, elevated transaction velocity, unrecognized device, and suspicious transaction location",
    "createdAt": "2026-11-16 19:20:11"
  }
]
```

---

## 5. Build and Execution Instructions

### Prerequisites
- CMake 3.15 or newer
- C++17 compatible compiler (GCC / MinGW / Clang / MSVC)
- Python / Ninja (recommended for fast builds)

### Step 1: Build the Project
Open a PowerShell terminal in the project root:

```powershell
# 1. Add compiler to PATH (using the self-contained toolchain):
$toolsBin = (Resolve-Path ".tools\w64devkit\bin").Path
$env:PATH = "$toolsBin;" + $env:PATH

# 2. Configure with CMake:
cmake -B backend\build -S backend -G "Ninja" -DCMAKE_C_COMPILER="$toolsBin\gcc.exe" -DCMAKE_CXX_COMPILER="$toolsBin\g++.exe" -DCMAKE_MAKE_PROGRAM="C:\Python314\Scripts\ninja.exe"

# 3. Compile:
cmake --build backend\build
```

### Step 2: Run Automated Tests
Execute the 8-scenario unit and integration test suite:

```powershell
.\backend\build\test_backend.exe
```

### Step 3: Start the Backend REST API Server
```powershell
.\backend\build\fraud_detection_backend.exe --port 8080 --db backend/database/fraud_detection.db --csv backend/dataset/transactions.csv
```

The server will start listening on `http://0.0.0.0:8080`.

### Step 4: Open the Frontend Dashboard
Simply open `index.html` in your web browser, or serve it using Python's simple HTTP server:

```powershell
python -m http.server 3000
```
Then visit: `http://localhost:3000` in your browser.

---

## 6. Project Division & Authorship

- **Student 1 (Backend & Database Developer)**:
  - Crow C++ REST API setup (`server.cpp`, `server.h`, `api.cpp`, `api.h`)
  - SQLite database schemas & prepared statements (`database.cpp`, `database.h`)
  - Dashboard statistics computation & alert storage
  - Frontend integration with JavaScript Fetch API

- **Student 2 (Fraud Detection Developer)**:
  - Multi-factor risk scoring engine & classification (`fraud_detector.cpp`, `fraud_detector.h`)
  - Feature extraction & explainable prediction reasoning (`model.cpp`, `model.h`)
  - Labelled benchmark transaction dataset (`dataset/transactions.csv`)
  - Comprehensive unit test validation suite (`tests/test_backend.cpp`)
