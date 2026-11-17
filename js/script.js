const API_BASE = "http://localhost:8080/api";

// DOM Elements
const serverStatusDot = document.getElementById("serverStatusDot");
const serverStatusText = document.getElementById("serverStatusText");

const statTotal = document.getElementById("statTotal");
const statFraud = document.getElementById("statFraud");
const statLegit = document.getElementById("statLegit");
const statRate = document.getElementById("statRate");
const statAvgScore = document.getElementById("statAvgScore");

const txForm = document.getElementById("txForm");
const submitBtn = document.getElementById("submitBtn");
const resultBox = document.getElementById("resultBox");

const alertsTableBody = document.getElementById("alertsTableBody");
const transactionsTableBody = document.getElementById("transactionsTableBody");
const toast = document.getElementById("toast");

// Initialize on page load
document.addEventListener("DOMContentLoaded", () => {
  checkServerHealth();
  loadStats();
  loadAlerts();
  loadTransactions();

  // Periodic polling for live server status & dashboard updates
  setInterval(checkServerHealth, 5000);
  setInterval(loadStats, 10000);
  setInterval(loadAlerts, 10000);
});

// Toast notification helper
function showToast(message, type = "success") {
  toast.textContent = message;
  toast.className = type;
  toast.style.display = "block";
  setTimeout(() => {
    toast.style.display = "none";
  }, 4000);
}

// -------------------------------------------------------------
// 1. Server Health Check (GET /api/health)
// -------------------------------------------------------------
async function checkServerHealth() {
  try {
    const res = await fetch(`${API_BASE}/health`, { method: "GET" });
    if (res.ok) {
      const data = await res.json();
      serverStatusDot.classList.add("online");
      serverStatusText.textContent = `Backend Online (${data.database || 'DB Connected'})`;
    } else {
      throw new Error("Server responded with error");
    }
  } catch (err) {
    serverStatusDot.classList.remove("online");
    serverStatusText.textContent = "Backend Offline (Port 8080)";
  }
}

// -------------------------------------------------------------
// 2. Load Dashboard Statistics (GET /api/stats)
// -------------------------------------------------------------
async function loadStats() {
  try {
    const res = await fetch(`${API_BASE}/stats`);
    if (!res.ok) return;
    const data = await res.json();

    statTotal.textContent = data.totalTransactions ?? 0;
    statFraud.textContent = data.totalFraud ?? 0;
    statLegit.textContent = data.totalLegitimate ?? 0;
    statRate.textContent = `${data.fraudPercentage ?? 0}%`;
    statAvgScore.textContent = data.averageRiskScore ?? 0;
  } catch (err) {
    console.warn("Could not fetch dashboard stats:", err.message);
  }
}

// -------------------------------------------------------------
// 3. Load Fraud Alerts (GET /api/alerts)
// -------------------------------------------------------------
async function loadAlerts() {
  try {
    const res = await fetch(`${API_BASE}/alerts?limit=6`);
    if (!res.ok) return;
    const alerts = await res.json();

    if (!alerts || alerts.length === 0) {
      alertsTableBody.innerHTML = `<tr><td colspan="5" style="text-align:center; color: var(--text-muted);">No active fraud alerts.</td></tr>`;
      return;
    }

    alertsTableBody.innerHTML = alerts.map(a => `
      <tr>
        <td><strong>${a.alertId}</strong></td>
        <td><code>${a.transactionId}</code></td>
        <td><span class="severity-tag ${a.severity}">${a.severity}</span></td>
        <td>${a.message}</td>
        <td style="color: var(--text-muted); font-size: 0.8rem;">${a.createdAt}</td>
      </tr>
    `).join("");
  } catch (err) {
    console.warn("Could not fetch alerts:", err.message);
  }
}

// -------------------------------------------------------------
// 4. Load Transactions History (GET /api/transactions)
// -------------------------------------------------------------
async function loadTransactions() {
  try {
    const res = await fetch(`${API_BASE}/transactions?limit=10`);
    if (!res.ok) return;
    const data = await res.json();
    const list = data.transactions || [];

    if (list.length === 0) {
      transactionsTableBody.innerHTML = `<tr><td colspan="7" style="text-align:center; color: var(--text-muted);">No transactions recorded yet.</td></tr>`;
      return;
    }

    transactionsTableBody.innerHTML = list.map(tx => `
      <tr>
        <td><code>${tx.id}</code></td>
        <td><strong>₹${Number(tx.amount).toLocaleString('en-IN')}</strong></td>
        <td>${tx.location}</td>
        <td>${tx.frequency}</td>
        <td>${tx.deviceRecognized ? "✅ Yes" : "❌ No"}</td>
        <td><strong>${tx.riskScore}</strong></td>
        <td><span class="status-badge ${tx.status}">${tx.status}</span></td>
        <td style="color: var(--text-muted); font-size: 0.8rem;">${tx.timestamp}</td>
      </tr>
    `).join("");
  } catch (err) {
    console.warn("Could not fetch transactions:", err.message);
  }
}

// -------------------------------------------------------------
// 5. Submit New Transaction (POST /api/transactions)
// -------------------------------------------------------------
txForm.addEventListener("submit", async (e) => {
  e.preventDefault();

  const amountVal = parseFloat(document.getElementById("amount").value);
  const locationVal = document.getElementById("location").value.trim();
  const freqVal = parseInt(document.getElementById("frequency").value, 10);
  const deviceVal = document.getElementById("deviceRecognized").checked;

  if (isNaN(amountVal) || amountVal <= 0) {
    showToast("Please enter a valid positive amount.", "error");
    return;
  }
  if (!locationVal) {
    showToast("Please specify transaction location.", "error");
    return;
  }
  if (isNaN(freqVal) || freqVal < 0) {
    showToast("Frequency must be 0 or higher.", "error");
    return;
  }

  const payload = {
    amount: amountVal,
    location: locationVal,
    frequency: freqVal,
    deviceRecognized: deviceVal
  };

  submitBtn.disabled = true;
  submitBtn.innerHTML = `<span>Evaluating Risk...</span>`;

  try {
    const res = await fetch(`${API_BASE}/transactions`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json"
      },
      body: JSON.stringify(payload)
    });

    const data = await res.json();

    if (!res.ok) {
      showToast(data.error || "Transaction evaluation failed", "error");
      renderErrorResult(data.error);
    } else {
      showToast(`Transaction ${data.transactionId} evaluated: ${data.status}!`, "success");
      renderSuccessResult(data);
      // Refresh dashboard data
      loadStats();
      loadAlerts();
      loadTransactions();
    }
  } catch (err) {
    showToast("Could not connect to C++ backend on port 8080.", "error");
    renderErrorResult("Network error: Ensure the C++ Crow backend is running on http://localhost:8080.");
  } finally {
    submitBtn.disabled = false;
    submitBtn.innerHTML = `<span>Check Transaction</span>`;
  }
});

function renderSuccessResult(res) {
  const isFraud = res.status === "FRAUD";
  let meterClass = "low";
  if (res.riskScore >= 75) {
    meterClass = "high";
  } else if (res.riskScore >= 50) {
    meterClass = "medium";
  }

  resultBox.innerHTML = `
    <div class="result-active">
      <div class="result-header">
        <span class="tx-id-badge">${res.transactionId}</span>
        <span class="status-badge ${res.status}">
          ${isFraud ? '⚠️ ' : '🛡️ '} ${res.status}
        </span>
      </div>

      <div class="meter-container">
        <div class="meter-info">
          <span>Risk Probability Score</span>
          <strong>${res.riskScore} / 100</strong>
        </div>
        <div class="meter-bar">
          <div class="meter-fill ${meterClass}" style="width: ${res.riskScore}%;"></div>
        </div>
      </div>

      <div class="reason-box ${res.status}">
        <strong>Prediction Explanation:</strong>
        <p style="margin-top: 4px; color: var(--text-main);">${res.reason}</p>
      </div>
    </div>
  `;
}

function renderErrorResult(msg) {
  resultBox.innerHTML = `
    <div style="color: var(--danger); text-align: center; padding: 20px;">
      <p style="font-weight: 600; margin-bottom: 6px;">Evaluation Error</p>
      <p style="font-size: 0.88rem; color: var(--text-muted);">${msg}</p>
    </div>
  `;
}

// -------------------------------------------------------------
// 6. Quick Presets Helper
// -------------------------------------------------------------
function setPreset(type) {
  const amountInput = document.getElementById("amount");
  const locInput = document.getElementById("location");
  const freqInput = document.getElementById("frequency");
  const devInput = document.getElementById("deviceRecognized");

  switch(type) {
    case 'normal':
      amountInput.value = "1250";
      locInput.value = "Delhi";
      freqInput.value = "1";
      devInput.checked = true;
      break;
    case 'high_amount':
      amountInput.value = "85000";
      locInput.value = "Mumbai";
      freqInput.value = "2";
      devInput.checked = false;
      break;
    case 'velocity':
      amountInput.value = "15000";
      locInput.value = "Bengaluru";
      freqInput.value = "8";
      devInput.checked = false;
      break;
    case 'suspicious_loc':
      amountInput.value = "72000";
      locInput.value = "Darknet Gateway";
      freqInput.value = "5";
      devInput.checked = false;
      break;
  }
}
