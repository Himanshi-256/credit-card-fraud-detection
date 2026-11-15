
const fraudForm = document.getElementById("fraudForm");
const result = document.getElementById("result");

fraudForm.addEventListener("submit", function(event) {
    event.preventDefault();

    const amount = Number(document.getElementById("amount").value);
    const location = document.getElementById("location").value.trim();
    const frequency = Number(document.getElementById("frequency").value);
    const device = document.getElementById("device").value;

    if (amount <= 0 || !location || frequency < 0) {
        result.textContent = "Please enter valid transaction details.";
        return;
    }

    // Simple demonstration rules, not an ML prediction
    let risk = 0;

    if (amount > 50000) risk += 2;
    if (frequency > 5) risk += 2;
    if (device === "no") risk += 2;

    if (risk >= 3) {
        result.innerHTML =
            '<strong style="color:#d64d4d">High Risk: Transaction Flagged</strong><br>' +
            'The transaction has suspicious indicators and requires review.';
    } else {
        result.innerHTML =
            '<strong style="color:#239c72">Low Risk: Transaction Passed</strong><br>' +
            'No major risk indicators were found by the demo rules.';
    }
});

// Transaction search

const searchInput = document.getElementById("searchInput");

searchInput.addEventListener("input", function() {
    const searchValue = this.value.toLowerCase();
    const rows = document.querySelectorAll("#transactionTable tr");

    rows.forEach(function(row) {
        row.style.display = row.textContent.toLowerCase()
            .includes(searchValue) ? "" : "none";
    });
});
