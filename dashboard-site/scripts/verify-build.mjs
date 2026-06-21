import { access, readFile } from "node:fs/promises";
import { resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const siteRoot = resolve(fileURLToPath(import.meta.url), "..", "..");
const distRoot = resolve(siteRoot, "dist");

const requiredFiles = [
  "server/index.js",
  "client/index.html",
  "client/styles.css",
  "client/app.js",
  ".openai/hosting.json"
];

for (const file of requiredFiles) {
  await access(resolve(distRoot, file));
}

const workerModule = await import(pathToFileURL(resolve(distRoot, "server/index.js")));
const worker = workerModule.default;

const response = await worker.fetch(new Request("https://vault-dashboard.example/"));
if (!response.ok) {
  throw new Error(`Expected 200 for dashboard root, got ${response.status}`);
}

const html = await response.text();
const requiredText = [
  "Vault Operations Dashboard",
  "Backup Overview",
  "Files protected",
  "Job Queue Status",
  "AI Security Findings",
  "Restore History",
  "Throughput Metrics"
];

for (const text of requiredText) {
  if (!html.includes(text)) {
    throw new Error(`Missing dashboard text in production HTML: ${text}`);
  }
}

const cssResponse = await worker.fetch(new Request("https://vault-dashboard.example/styles.css"));
if (!cssResponse.ok || !cssResponse.headers.get("content-type")?.includes("text/css")) {
  throw new Error("Expected stylesheet asset to be served with text/css");
}

const appJs = await readFile(resolve(distRoot, "client", "app.js"), "utf8");
if (!appJs.includes("securityFindings") || !appJs.includes("throughput")) {
  throw new Error("Expected demo security and throughput metadata in app.js");
}

console.log("Production dashboard build verified.");
