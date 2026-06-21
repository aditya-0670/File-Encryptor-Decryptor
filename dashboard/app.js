(function () {
  const backupOverview = [
    { label: "Completed backups", value: "12,804", detail: "96.8% of scheduled workload" },
    { label: "Protected namespaces", value: "48", detail: "finance, product, legal, infra" },
    { label: "Encryption failures", value: "0", detail: "AES-GCM tag write path clear" },
    { label: "Oldest recovery point", value: "31d", detail: "policy target: 30d minimum" }
  ];

  const queueLanes = [
    { name: "Backup ingest", percent: 78, amount: "312 running", type: "backup" },
    { name: "Restore verification", percent: 44, amount: "64 running", type: "restore" },
    { name: "Integrity scan", percent: 63, amount: "1,147 queued", type: "verify" }
  ];

  const securityFindings = [
    {
      title: "Recovery policy drift in legal-archive",
      severity: "high",
      meta: "14 paths exceed target recovery point by 6h 18m"
    },
    {
      title: "Unusual restore burst from eu-west-2",
      severity: "medium",
      meta: "19 restores from two operators in 42 minutes"
    },
    {
      title: "Low dedupe on media-research",
      severity: "low",
      meta: "Chunk similarity dropped to 8.2% after ingest surge"
    }
  ];

  const failedJobs = [
    { id: "job-8f42", namespace: "ml-training", reason: "source read timeout", age: "11m" },
    { id: "job-18aa", namespace: "finance-ledger", reason: "node quorum lost", age: "24m" },
    { id: "job-5d70", namespace: "legal-archive", reason: "retention policy conflict", age: "47m" },
    { id: "job-01c4", namespace: "infra-state", reason: "manifest lock expired", age: "1h 12m" }
  ];

  const restoreHistory = [
    { title: "finance-ledger / q2-close", status: "verified", meta: "2.1 TB restored to us-east-1 in 38m" },
    { title: "product-media / launch-assets", status: "verified", meta: "418 GB restored to eu-central-1 in 9m" },
    { title: "infra-state / control-plane", status: "verified", meta: "74 GB restored to ap-south-1 in 5m" },
    { title: "legal-archive / discovery-927", status: "verified", meta: "1.8 TB restored to isolated review vault" }
  ];

  const throughput = {
    labels: ["00:00", "04:00", "08:00", "12:00", "16:00", "20:00", "24:00"],
    write: [680, 720, 810, 940, 890, 1030, 970],
    restore: [210, 260, 340, 390, 370, 440, 410]
  };

  function renderBackupStats() {
    const target = document.getElementById("backupStats");
    target.innerHTML = backupOverview.map((item) => `
      <div class="backup-row">
        <div>
          <strong>${item.label}</strong>
          <span>${item.detail}</span>
        </div>
        <strong>${item.value}</strong>
      </div>
    `).join("");
  }

  function renderQueueLanes() {
    const target = document.getElementById("queueLanes");
    target.innerHTML = queueLanes.map((lane) => `
      <div class="lane">
        <div class="lane-top">
          <span>${lane.name}</span>
          <span>${lane.amount}</span>
        </div>
        <div class="lane-track" aria-label="${lane.name} ${lane.percent} percent active">
          <div class="lane-fill ${lane.type}" style="width: ${lane.percent}%"></div>
        </div>
      </div>
    `).join("");
  }

  function renderSecurityFindings() {
    const target = document.getElementById("securityFindings");
    target.innerHTML = securityFindings.map((finding) => `
      <div class="finding-item">
        <div class="finding-title">
          <span>${finding.title}</span>
          <span class="severity ${finding.severity}">${finding.severity}</span>
        </div>
        <div class="finding-meta">${finding.meta}</div>
      </div>
    `).join("");
  }

  function renderFailedJobs() {
    const target = document.getElementById("failedJobs");
    target.innerHTML = failedJobs.map((job) => `
      <tr>
        <td><span class="job-id">${job.id}</span></td>
        <td>${job.namespace}</td>
        <td><span class="reason">${job.reason}</span></td>
        <td>${job.age}</td>
      </tr>
    `).join("");
  }

  function renderRestoreHistory() {
    const target = document.getElementById("restoreHistory");
    target.innerHTML = restoreHistory.map((item) => `
      <div class="restore-item">
        <div class="restore-title">
          <span>${item.title}</span>
          <span class="restore-status">${item.status}</span>
        </div>
        <div class="restore-meta">${item.meta}</div>
      </div>
    `).join("");
  }

  function setupCanvas(canvas) {
    const ratio = window.devicePixelRatio || 1;
    const rect = canvas.getBoundingClientRect();
    const width = Math.max(1, Math.round(rect.width * ratio));
    const height = Math.max(1, Math.round(rect.height * ratio));
    if (canvas.width !== width || canvas.height !== height) {
      canvas.width = width;
      canvas.height = height;
    }
    const ctx = canvas.getContext("2d");
    ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
    return { ctx, width: rect.width, height: rect.height };
  }

  function drawBackupRing() {
    const canvas = document.getElementById("backupRing");
    const { ctx, width, height } = setupCanvas(canvas);
    const centerX = width / 2;
    const centerY = height / 2;
    const radius = Math.min(width, height) / 2 - 16;
    const completed = 0.968;
    const segments = [
      { value: completed, color: "#1f8f68" },
      { value: 1 - completed, color: "#e1e8e4" }
    ];

    ctx.clearRect(0, 0, width, height);
    ctx.lineWidth = 16;
    ctx.lineCap = "round";

    let start = -Math.PI / 2;
    segments.forEach((segment) => {
      const end = start + Math.PI * 2 * segment.value;
      ctx.beginPath();
      ctx.strokeStyle = segment.color;
      ctx.arc(centerX, centerY, radius, start, end);
      ctx.stroke();
      start = end + 0.04;
    });
  }

  function drawGrid(ctx, width, height, padding) {
    ctx.strokeStyle = "#e0e8e4";
    ctx.lineWidth = 1;
    for (let index = 0; index < 4; index += 1) {
      const y = padding.top + ((height - padding.top - padding.bottom) / 3) * index;
      ctx.beginPath();
      ctx.moveTo(padding.left, y);
      ctx.lineTo(width - padding.right, y);
      ctx.stroke();
    }
  }

  function drawLine(ctx, points, color) {
    ctx.strokeStyle = color;
    ctx.lineWidth = 3;
    ctx.lineJoin = "round";
    ctx.lineCap = "round";
    ctx.beginPath();
    points.forEach((point, index) => {
      if (index === 0) {
        ctx.moveTo(point.x, point.y);
      } else {
        ctx.lineTo(point.x, point.y);
      }
    });
    ctx.stroke();

    points.forEach((point) => {
      ctx.beginPath();
      ctx.fillStyle = "#ffffff";
      ctx.strokeStyle = color;
      ctx.lineWidth = 2;
      ctx.arc(point.x, point.y, 4, 0, Math.PI * 2);
      ctx.fill();
      ctx.stroke();
    });
  }

  function drawThroughputChart() {
    const canvas = document.getElementById("throughputChart");
    const { ctx, width, height } = setupCanvas(canvas);
    const padding = { top: 16, right: 18, bottom: 30, left: 42 };
    const values = throughput.write.concat(throughput.restore);
    const max = Math.max(...values) * 1.15;
    const min = 0;

    const mapPoints = (series) => series.map((value, index) => {
      const x = padding.left + ((width - padding.left - padding.right) / (series.length - 1)) * index;
      const y = height - padding.bottom - ((value - min) / (max - min)) * (height - padding.top - padding.bottom);
      return { x, y, value };
    });

    ctx.clearRect(0, 0, width, height);
    drawGrid(ctx, width, height, padding);
    drawLine(ctx, mapPoints(throughput.write), "#1f8f68");
    drawLine(ctx, mapPoints(throughput.restore), "#217f92");

    ctx.fillStyle = "#62706a";
    ctx.font = "12px Inter, system-ui, sans-serif";
    throughput.labels.forEach((label, index) => {
      const x = padding.left + ((width - padding.left - padding.right) / (throughput.labels.length - 1)) * index;
      ctx.fillText(label, Math.max(0, x - 16), height - 8);
    });

    ctx.fillText("GB/h", 0, 14);
  }

  function drawCharts() {
    drawBackupRing();
    drawThroughputChart();
  }

  function init() {
    renderBackupStats();
    renderQueueLanes();
    renderSecurityFindings();
    renderFailedJobs();
    renderRestoreHistory();
    drawCharts();
  }

  window.addEventListener("resize", drawCharts);
  window.addEventListener("DOMContentLoaded", init);
})();
