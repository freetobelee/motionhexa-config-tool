#!/usr/bin/env node
// Motionhexa local console: a zero-dependency Node server that reads/writes the
// real project source files directly on disk (no browser sandbox involved) and
// shells out to PlatformIO for build/deploy. Run with `node server.js` from
// anywhere -- paths below are resolved relative to this file, not to cwd.

"use strict";

const http = require("http");
const fs = require("fs");
const path = require("path");
const { spawn } = require("child_process");

const PROJECT_ROOT = path.resolve(__dirname, "..", "..");
const MAIN_CPP = path.join(PROJECT_ROOT, "src", "main.cpp");
const PORT = process.env.PORT ? Number(process.env.PORT) : 4173;
const PIO_ENV = "v5";

// Files scanned for `@tunable("Label", min, max)` tags. Add a path here to
// make its tagged constants show up in the Config panel -- no other code
// change needed, the scanner is generic.
const TUNABLE_FILES = [
  path.join(PROJECT_ROOT, "src", "main.cpp"),
  path.join(PROJECT_ROOT, "src", "power.h"),
  path.join(PROJECT_ROOT, "src", "patterns.h"),
];

/* ---------------- pattern registration block (main.cpp) ---------------- */

const BLOCK_START_RE = /patternManager\.registerPattern<MotionHexa>\(\);/;
const BLOCK_END_RE = /^\s*#if HARDWARE_VERSION >= 3\s*$/;
const ACTIVE_LINE_RE = /^\s*patternManager\.registerPattern<(\w+)>\(\);\s*$/;
const DISABLED_LINE_RE = /^\s*\/\/\s*(\w+)\s*\(see patterns\.h\)\s*is left defined but unregistered for now\.\s*$/;

function findPatternBlock(lines) {
  let startIdx = -1;
  for (let i = 0; i < lines.length; i++) {
    if (BLOCK_START_RE.test(lines[i])) { startIdx = i; break; }
  }
  if (startIdx === -1) throw new Error("Could not find the pattern registration block (MotionHexa marker) in main.cpp -- has the file structure changed?");

  let endMarkerIdx = -1;
  for (let i = startIdx; i < lines.length; i++) {
    if (BLOCK_END_RE.test(lines[i])) { endMarkerIdx = i; break; }
  }
  if (endMarkerIdx === -1) throw new Error("Could not find the '#if HARDWARE_VERSION >= 3' end marker after the pattern block in main.cpp.");

  // last non-blank line before the end marker is the last pattern line
  let endIdx = endMarkerIdx - 1;
  while (endIdx > startIdx && lines[endIdx].trim() === "") endIdx--;

  return { startIdx, endIdx };
}

function parsePatterns() {
  const text = fs.readFileSync(MAIN_CPP, "utf8");
  const lines = text.split("\n");
  const { startIdx, endIdx } = findPatternBlock(lines);

  const patterns = [];
  for (let i = startIdx; i <= endIdx; i++) {
    const line = lines[i];
    let m = line.match(ACTIVE_LINE_RE);
    if (m) { patterns.push({ name: m[1], enabled: true }); continue; }
    m = line.match(DISABLED_LINE_RE);
    if (m) { patterns.push({ name: m[1], enabled: false }); continue; }
    throw new Error("Unrecognized line inside the pattern block (line " + (i + 1) + "): " + JSON.stringify(line) +
      " -- refusing to touch main.cpp until this is understood.");
  }
  return patterns;
}

function writePatterns(newPatterns) {
  const text = fs.readFileSync(MAIN_CPP, "utf8");
  const eol = text.indexOf("\r\n") !== -1 ? "\r\n" : "\n";
  const lines = text.split(/\r\n|\n/);
  const { startIdx, endIdx } = findPatternBlock(lines);

  const seen = new Set();
  newPatterns.forEach(function (p) {
    if (!/^\w+$/.test(p.name)) throw new Error("Invalid pattern name: " + JSON.stringify(p.name));
    if (seen.has(p.name)) throw new Error("Duplicate pattern name in submitted list: " + p.name);
    seen.add(p.name);
  });
  const existingNames = new Set(parsePatterns().map(function (p) { return p.name; }));
  if (newPatterns.length !== existingNames.size || ![...existingNames].every(function (n) { return seen.has(n); })) {
    throw new Error("Submitted pattern list doesn't match the set of patterns currently in main.cpp -- refusing to write (this tool only reorders/toggles existing patterns, it doesn't add or remove them).");
  }

  const newLines = newPatterns.map(function (p) {
    return p.enabled
      ? "  patternManager.registerPattern<" + p.name + ">();"
      : "  // " + p.name + " (see patterns.h) is left defined but unregistered for now.";
  });

  const result = lines.slice(0, startIdx).concat(newLines, lines.slice(endIdx + 1));
  fs.writeFileSync(MAIN_CPP, result.join(eol), "utf8");
}

/* ---------------- @tunable config scanner ---------------- */

const TUNABLE_RE = /=\s*(-?\d+(?:\.\d+)?)\s*;.*@tunable\(\s*"([^"]+)"\s*,\s*(-?\d+(?:\.\d+)?)\s*,\s*(-?\d+(?:\.\d+)?)\s*\)/;

function scanTunables() {
  const out = [];
  TUNABLE_FILES.forEach(function (filePath) {
    if (!fs.existsSync(filePath)) return;
    const text = fs.readFileSync(filePath, "utf8");
    const lines = text.split(/\r\n|\n/);
    lines.forEach(function (line, i) {
      const m = line.match(TUNABLE_RE);
      if (!m) return;
      out.push({
        id: path.relative(PROJECT_ROOT, filePath) + ":" + (i + 1),
        file: path.relative(PROJECT_ROOT, filePath),
        line: i + 1,
        label: m[2],
        value: Number(m[1]),
        min: Number(m[3]),
        max: Number(m[4]),
      });
    });
  });
  return out;
}

function writeTunable(id, value) {
  const parts = id.split(":");
  const lineNum = Number(parts.pop());
  const relFile = parts.join(":");
  const filePath = path.resolve(PROJECT_ROOT, relFile);
  if (!TUNABLE_FILES.includes(filePath)) throw new Error("Refusing to write to a file outside the tunable allowlist: " + relFile);
  if (!Number.isFinite(value)) throw new Error("Invalid tunable value: " + value);

  const text = fs.readFileSync(filePath, "utf8");
  const eol = text.indexOf("\r\n") !== -1 ? "\r\n" : "\n";
  const lines = text.split(/\r\n|\n/);
  const idx = lineNum - 1;
  if (idx < 0 || idx >= lines.length) throw new Error("Tunable line number out of range: " + id);
  const line = lines[idx];
  const m = line.match(TUNABLE_RE);
  if (!m) throw new Error("Line " + lineNum + " in " + relFile + " no longer matches the expected @tunable pattern -- refusing to write (did the file change?).");

  const newLine = line.replace(/=\s*-?\d+(?:\.\d+)?\s*;/, "= " + value + ";");
  lines[idx] = newLine;
  fs.writeFileSync(filePath, lines.join(eol), "utf8");
}

/* ---------------- build / deploy (streamed) ---------------- */

function runPio(args, res) {
  res.writeHead(200, { "Content-Type": "text/event-stream", "Cache-Control": "no-cache", Connection: "keep-alive" });
  const send = function (event, data) { res.write("event: " + event + "\ndata: " + JSON.stringify(data) + "\n\n"); };

  send("log", "$ pio " + args.join(" ") + "  (cwd: " + PROJECT_ROOT + ")");
  const child = spawn("pio", args, { cwd: PROJECT_ROOT });

  child.stdout.on("data", function (chunk) { send("log", chunk.toString()); });
  child.stderr.on("data", function (chunk) { send("log", chunk.toString()); });
  child.on("error", function (err) {
    send("log", "Failed to launch pio: " + err.message + " -- is PlatformIO's CLI on PATH for this shell?");
    send("done", { code: -1 });
    res.end();
  });
  child.on("close", function (code) {
    send("done", { code: code });
    res.end();
  });
}

/* ---------------- HTTP server ---------------- */

function sendJson(res, status, obj) {
  const body = JSON.stringify(obj);
  res.writeHead(status, { "Content-Type": "application/json", "Content-Length": Buffer.byteLength(body) });
  res.end(body);
}

function readBody(req) {
  return new Promise(function (resolve, reject) {
    let chunks = [];
    req.on("data", function (c) { chunks.push(c); });
    req.on("end", function () {
      try { resolve(chunks.length ? JSON.parse(Buffer.concat(chunks).toString("utf8")) : {}); }
      catch (e) { reject(new Error("Invalid JSON body: " + e.message)); }
    });
    req.on("error", reject);
  });
}

const MIME = { ".html": "text/html", ".js": "application/javascript", ".css": "text/css" };

const server = http.createServer(function (req, res) {
  const url = new URL(req.url, "http://localhost");

  try {
    if (req.method === "GET" && url.pathname === "/api/patterns") {
      return sendJson(res, 200, { patterns: parsePatterns() });
    }
    if (req.method === "POST" && url.pathname === "/api/patterns") {
      return readBody(req).then(function (body) {
        writePatterns(body.patterns);
        sendJson(res, 200, { ok: true, patterns: parsePatterns() });
      }).catch(function (e) { sendJson(res, 400, { error: e.message }); });
    }
    if (req.method === "GET" && url.pathname === "/api/config") {
      return sendJson(res, 200, { tunables: scanTunables() });
    }
    if (req.method === "POST" && url.pathname === "/api/config") {
      return readBody(req).then(function (body) {
        (body.updates || []).forEach(function (u) { writeTunable(u.id, Number(u.value)); });
        sendJson(res, 200, { ok: true, tunables: scanTunables() });
      }).catch(function (e) { sendJson(res, 400, { error: e.message }); });
    }
    if (req.method === "POST" && url.pathname === "/api/build") {
      return runPio(["run", "-e", PIO_ENV], res);
    }
    if (req.method === "POST" && url.pathname === "/api/deploy") {
      return runPio(["run", "-e", PIO_ENV, "-t", "upload"], res);
    }

    // static file serving for the UI
    let filePath = url.pathname === "/" ? "/index.html" : url.pathname;
    filePath = path.join(__dirname, "public", path.normalize(filePath).replace(/^(\.\.[\/\\])+/, ""));
    fs.readFile(filePath, function (err, data) {
      if (err) { res.writeHead(404); res.end("Not found"); return; }
      const ext = path.extname(filePath);
      res.writeHead(200, { "Content-Type": MIME[ext] || "application/octet-stream" });
      res.end(data);
    });
  } catch (e) {
    sendJson(res, 500, { error: e.message });
  }
});

server.listen(PORT, function () {
  console.log("Motionhexa console running at http://localhost:" + PORT);
  console.log("Project root: " + PROJECT_ROOT);
});
