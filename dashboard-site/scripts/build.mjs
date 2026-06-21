import { access, cp, mkdir, readFile, rm, writeFile, copyFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const siteRoot = resolve(here, "..");
const repoRoot = resolve(siteRoot, "..");
const packagedSourceRoot = resolve(siteRoot, "source");
const repoSourceRoot = resolve(repoRoot, "dashboard");
const publicRoot = resolve(siteRoot, "public");
const distRoot = resolve(siteRoot, "dist");
const clientRoot = resolve(distRoot, "client");
const serverRoot = resolve(distRoot, "server");
const openaiRoot = resolve(distRoot, ".openai");

const assets = [
  { route: "/index.html", file: "index.html", contentType: "text/html; charset=utf-8" },
  { route: "/styles.css", file: "styles.css", contentType: "text/css; charset=utf-8" },
  { route: "/app.js", file: "app.js", contentType: "application/javascript; charset=utf-8" }
];

async function exists(path) {
  try {
    await access(path);
    return true;
  } catch {
    return false;
  }
}

const sourceRoot = (await exists(packagedSourceRoot)) ? packagedSourceRoot : repoSourceRoot;

await rm(distRoot, { recursive: true, force: true });
await mkdir(clientRoot, { recursive: true });
await mkdir(serverRoot, { recursive: true });
await mkdir(openaiRoot, { recursive: true });

const manifestEntries = [];
for (const asset of assets) {
  const sourcePath = resolve(sourceRoot, asset.file);
  const outputPath = resolve(clientRoot, asset.file);
  const bytes = await readFile(sourcePath);
  await writeFile(outputPath, bytes);
  manifestEntries.push({
    route: asset.route,
    contentType: asset.contentType,
    base64: bytes.toString("base64")
  });
}

await copyFile(resolve(siteRoot, ".openai", "hosting.json"), resolve(openaiRoot, "hosting.json"));
await cp(publicRoot, clientRoot, { recursive: true, force: true }).catch((error) => {
  if (error.code !== "ENOENT") {
    throw error;
  }
});

const serverSource = `const ASSETS = ${JSON.stringify(manifestEntries, null, 2)};

function decodeBase64(value) {
  const binary = atob(value);
  const bytes = new Uint8Array(binary.length);
  for (let index = 0; index < binary.length; index += 1) {
    bytes[index] = binary.charCodeAt(index);
  }
  return bytes;
}

function assetForPath(pathname) {
  const normalized = pathname === "/" ? "/index.html" : pathname;
  return ASSETS.find((asset) => asset.route === normalized);
}

export default {
  async fetch(request) {
    const url = new URL(request.url);
    const asset = assetForPath(url.pathname);

    if (!asset) {
      return new Response("Not found", {
        status: 404,
        headers: {
          "content-type": "text/plain; charset=utf-8",
          "x-content-type-options": "nosniff"
        }
      });
    }

    return new Response(decodeBase64(asset.base64), {
      headers: {
        "content-type": asset.contentType,
        "cache-control": asset.route === "/index.html" ? "no-cache" : "public, max-age=31536000, immutable",
        "x-content-type-options": "nosniff",
        "referrer-policy": "no-referrer",
        "permissions-policy": "camera=(), microphone=(), geolocation=()"
      }
    });
  }
};
`;

await writeFile(resolve(serverRoot, "index.js"), serverSource);

console.log(`Built dashboard site at ${distRoot}`);
