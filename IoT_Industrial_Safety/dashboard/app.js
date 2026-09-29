/**
 * Industrial Safety Monitor Dashboard
 * Retrieves data from ThingSpeak public API.
 */

// ==========================================
// CONFIGURATION
// ==========================================
const CONFIG = {
    channelId: "3513219",             // ThingSpeak Channel ID (public metadata)
    readApiKey: "YOUR_THINGSPEAK_READ_API_KEY",   // Leave "" if channel is public
    updateInterval: 15000,            // Update every 15 seconds
    staleTimeout: 60000               // Consider offline after 60 seconds of no data
};

// ==========================================
// GLOBALS & CHART INSTANCES
// ==========================================
let tempChart, humChart, gasChart, statusChart;
let lastEntryId = 0;
let lastUpdateTime = new Date(0);

// Colors
const COLOR_NORMAL = '#10b981';
const COLOR_WARNING = '#f59e0b';
const COLOR_CRITICAL = '#ef4444';
const COLOR_GRID = '#334155';
const COLOR_TEXT = '#94a3b8';

// ==========================================
// INIT
// ==========================================
document.addEventListener('DOMContentLoaded', () => {
    initCharts();
    fetchData(); // Initial fetch
    
    // Set auto refresh
    setInterval(fetchData, CONFIG.updateInterval);
    // Check stale status more frequently
    setInterval(checkStaleData, 5000);
});

// ==========================================
// FETCH DATA
// ==========================================
async function fetchData() {
    if (CONFIG.channelId === "YOUR_CHANNEL_ID" || !CONFIG.channelId) {
        setConnectionStatus('OFFLINE', 'Please configure Channel ID in app.js');
        return;
    }

    const hasReadApiKey = CONFIG.readApiKey && CONFIG.readApiKey !== "YOUR_THINGSPEAK_READ_API_KEY";
    const url = `https://api.thingspeak.com/channels/${CONFIG.channelId}/feeds.json?results=60${hasReadApiKey ? '&api_key=' + CONFIG.readApiKey : ''}`;

    try {
        const response = await fetch(url);
        if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
        
        const data = await response.json();
        if (data.feeds && data.feeds.length > 0) {
            updateDashboard(data.feeds);
            lastUpdateTime = new Date(data.feeds[data.feeds.length - 1].created_at);
            setConnectionStatus('ONLINE', 'Connected');
            document.getElementById('last-update').innerText = `Last update: ${lastUpdateTime.toLocaleTimeString()}`;
        }
    } catch (error) {
        console.error("Error fetching data:", error);
        setConnectionStatus('OFFLINE', 'Cloud Offline');
    }
}

function checkStaleData() {
    const now = new Date();
    const diff = now - lastUpdateTime;
    if (diff > CONFIG.staleTimeout && lastUpdateTime.getTime() !== 0) {
        setConnectionStatus('OFFLINE', 'Data Stale (Device Offline?)');
        document.getElementById('val-cloud').innerText = 'OFFLINE';
        document.getElementById('val-cloud').className = 'sensor-value text-offline';
    }
}

// ==========================================
// UPDATE UI
// ==========================================
function updateDashboard(feeds) {
    const latest = feeds[feeds.length - 1];

    // 1. Parse Fields
    const temp = parseFloat(latest.field1) || 0;
    const hum = parseFloat(latest.field2) || 0;
    const gas = parseInt(latest.field3) || 0;
    const flame = parseInt(latest.field4) || 0;
    const vib = parseInt(latest.field5) || 0;
    const touch = parseInt(latest.field6) || 0;
    const sysStatus = parseInt(latest.field7) || 0;
    const eventCode = parseInt(latest.field8) || 0;

    // 2. Update Sensor Cards
    document.getElementById('val-temp').innerText = latest.field1 ? temp.toFixed(1) : 'N/A';
    document.getElementById('val-hum').innerText = latest.field2 ? hum.toFixed(0) : 'N/A';
    document.getElementById('val-gas').innerText = latest.field3 ? gas : 'N/A';
    
    const valFlame = document.getElementById('val-flame');
    if (flame === 1) {
        valFlame.innerText = 'DETECTED';
        valFlame.className = 'sensor-value text-critical';
    } else {
        valFlame.innerText = 'SAFE';
        valFlame.className = 'sensor-value text-normal';
    }

    const valVib = document.getElementById('val-vib');
    if (vib === 2) {
        valVib.innerText = 'CRITICAL';
        valVib.className = 'sensor-value text-critical';
    } else if (vib === 1) {
        valVib.innerText = 'WARNING';
        valVib.className = 'sensor-value text-warning';
    } else {
        valVib.innerText = 'NORMAL';
        valVib.className = 'sensor-value text-normal';
    }

    const valTouch = document.getElementById('val-touch');
    if (touch === 1) {
        valTouch.innerText = 'ACTIVE';
        valTouch.className = 'sensor-value text-critical';
    } else {
        valTouch.innerText = 'OFF';
        valTouch.className = 'sensor-value text-normal';
    }

    // 3. Update Main Status & Alert Panel
    updateMainStatus(sysStatus);

    // 4. Update Event History
    if (latest.entry_id > lastEntryId) {
        lastEntryId = latest.entry_id;
        if (eventCode > 0) {
            addEventToHistory(latest.created_at, eventCode);
        }
    }

    // 5. Update Charts
    updateCharts(feeds);
}

function updateMainStatus(status) {
    const card = document.getElementById('main-status-card');
    const icon = document.getElementById('sys-icon');
    const text = document.getElementById('sys-text');
    const subtext = document.getElementById('sys-subtext');
    const alertPanel = document.getElementById('alert-content');

    card.className = 'card main-status';

    if (status === 0) {
        card.classList.add('normal');
        icon.innerText = '🟢';
        text.innerText = 'NORMAL';
        text.className = 'status-text text-normal';
        subtext.innerText = 'System operating normally';
        alertPanel.innerHTML = '<p class="text-normal">No active alerts</p>';
    } else if (status === 1) {
        card.classList.add('warning');
        icon.innerText = '🟡';
        text.innerText = 'WARNING';
        text.className = 'status-text text-warning';
        subtext.innerText = 'Abnormal condition detected';
        alertPanel.innerHTML = '<p class="text-warning">⚠️ Warning condition detected</p>';
    } else if (status === 2) {
        card.classList.add('critical');
        icon.innerText = '🔴';
        text.innerText = 'CRITICAL';
        text.className = 'status-text text-critical';
        subtext.innerText = 'Emergency condition detected';
        alertPanel.innerHTML = `
            <h4 class="text-critical">🚨 Critical condition detected</h4>
            <p>Immediate local response active.</p>
        `;
    }
}

function addEventToHistory(timestamp, code) {
    const events = [
        "No Event",
        "High Temperature",
        "High Gas",
        "Flame Detected",
        "Vibration Escalation",
        "Manual Emergency",
        "Multiple Critical Conditions"
    ];

    const timeStr = new Date(timestamp).toLocaleTimeString();
    const eventName = events[code] || "Unknown Event";
    
    const list = document.getElementById('event-list');
    
    // Remove "Loading" if present
    if (list.innerHTML.includes('Loading')) list.innerHTML = '';

    const html = `
        <div class="event-item">
            <span class="event-time">${timeStr}</span>
            <span class="event-desc">${eventName}</span>
        </div>
    `;
    
    list.insertAdjacentHTML('afterbegin', html);
    
    // Keep only last 10
    while (list.children.length > 10) {
        list.removeChild(list.lastChild);
    }
}

function setConnectionStatus(status, text) {
    const dot = document.getElementById('conn-dot');
    const label = document.getElementById('conn-status');
    const cloudVal = document.getElementById('val-cloud');

    label.innerText = text;
    cloudVal.innerText = status;

    if (status === 'ONLINE') {
        dot.className = 'status-dot online';
        cloudVal.className = 'sensor-value text-normal';
    } else {
        dot.className = 'status-dot offline';
        cloudVal.className = 'sensor-value text-offline';
    }
}

// ==========================================
// CHARTS SETUP & UPDATE
// ==========================================
const chartOptions = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: { legend: { display: false } },
    scales: {
        x: { 
            ticks: { color: COLOR_TEXT, maxTicksLimit: 6 },
            grid: { color: COLOR_GRID }
        },
        y: { 
            ticks: { color: COLOR_TEXT },
            grid: { color: COLOR_GRID }
        }
    },
    elements: {
        point: { radius: 0 },
        line: { tension: 0.2, borderWidth: 2 }
    }
};

function initCharts() {
    const ctxTemp = document.getElementById('tempChart').getContext('2d');
    const ctxHum = document.getElementById('humChart').getContext('2d');
    const ctxGas = document.getElementById('gasChart').getContext('2d');
    const ctxStatus = document.getElementById('statusChart').getContext('2d');

    tempChart = new Chart(ctxTemp, {
        type: 'line',
        data: { labels: [], datasets: [{ data: [], borderColor: '#ef4444' }] },
        options: chartOptions
    });

    humChart = new Chart(ctxHum, {
        type: 'line',
        data: { labels: [], datasets: [{ data: [], borderColor: '#3b82f6' }] },
        options: chartOptions
    });

    gasChart = new Chart(ctxGas, {
        type: 'line',
        data: { labels: [], datasets: [{ data: [], borderColor: '#8b5cf6' }] },
        options: chartOptions
    });

    statusChart = new Chart(ctxStatus, {
        type: 'line',
        data: { labels: [], datasets: [{ data: [], borderColor: '#10b981', stepped: true }] },
        options: {
            ...chartOptions,
            scales: {
                ...chartOptions.scales,
                y: {
                    ...chartOptions.scales.y,
                    min: 0, max: 2,
                    ticks: {
                        color: COLOR_TEXT,
                        callback: function(value) {
                            if (value === 0) return 'NORMAL';
                            if (value === 1) return 'WARNING';
                            if (value === 2) return 'CRITICAL';
                            return '';
                        }
                    }
                }
            }
        }
    });
}

function updateCharts(feeds) {
    const labels = feeds.map(f => {
        const d = new Date(f.created_at);
        return `${d.getHours().toString().padStart(2,'0')}:${d.getMinutes().toString().padStart(2,'0')}`;
    });

    const tempData = feeds.map(f => parseFloat(f.field1) || null);
    const humData = feeds.map(f => parseFloat(f.field2) || null);
    const gasData = feeds.map(f => parseInt(f.field3) || null);
    const statusData = feeds.map(f => parseInt(f.field7) || 0);

    // Update Temp
    tempChart.data.labels = labels;
    tempChart.data.datasets[0].data = tempData;
    tempChart.update('none');

    // Update Hum
    humChart.data.labels = labels;
    humChart.data.datasets[0].data = humData;
    humChart.update('none');

    // Update Gas
    gasChart.data.labels = labels;
    gasChart.data.datasets[0].data = gasData;
    gasChart.update('none');

    // Update Status
    statusChart.data.labels = labels;
    statusChart.data.datasets[0].data = statusData;
    
    // Change color based on current status for the status chart line
    const currentStatus = statusData[statusData.length - 1];
    if (currentStatus === 2) statusChart.data.datasets[0].borderColor = COLOR_CRITICAL;
    else if (currentStatus === 1) statusChart.data.datasets[0].borderColor = COLOR_WARNING;
    else statusChart.data.datasets[0].borderColor = COLOR_NORMAL;
    
    statusChart.update('none');
}
