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
const PORT = process.env.PORT ? Number(process.env.PORT) : 2710; // 271 LEDs, x10 to stay above macOS's privileged-port line (<1024 needs admin rights)
const PIO_ENV = "v5";

// Files scanned for `@tunable("Label", min, max)` or `@tunable("Label", min,
// max, "PatternName")` tags. Add a path here to make its tagged constants
// show up in the Config/effect-settings panels -- no other code change
// needed, the scanner is generic. The optional 4th argument ties a tunable
// to a specific effect's settings page; omit it for a global/general setting.
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

/* ---------------- glyphs / elements (Hexa Object Forge, saves straight to patterns.h) ---------------- */

const PATTERNS_H = path.join(PROJECT_ROOT, "src", "patterns.h");

const SIZES = {
  xs: { count: 61, radius: 4, label: "Small" },
  md: { count: 127, radius: 6, label: "Medium" },
  lg: { count: 271, radius: 9, label: "Large" },
};

// must match the SYMBOLS table baked into the Hexa Object Forge tool exactly
const SYMBOLS = [
  { ch: " ", id: "SPACE" }, { ch: ".", id: "PERIOD" }, { ch: ",", id: "COMMA" },
  { ch: ":", id: "COLON" }, { ch: ";", id: "SEMI" }, { ch: "!", id: "BANG" },
  { ch: "?", id: "QMARK" }, { ch: "-", id: "DASH" }, { ch: "+", id: "PLUS" },
  { ch: "=", id: "EQUALS" }, { ch: "/", id: "SLASH" }, { ch: "\\", id: "BACKSLASH" }, { ch: "(", id: "LPAREN" },
  { ch: ")", id: "RPAREN" }, { ch: "°", id: "DEGREE" }, { ch: "&", id: "AMP" },
  { ch: "'", id: "APOS" },
];

function identForChar(ch) {
  if (/^[A-Za-z0-9]$/.test(ch)) return ch;
  const found = SYMBOLS.find(function (s) { return s.ch === ch; });
  return found ? found.id : "CH" + ch.charCodeAt(0);
}
function identToChar(ident) {
  if (/^[A-Za-z0-9]$/.test(ident)) return ident;
  const found = SYMBOLS.find(function (s) { return s.id === ident; });
  return found ? found.ch : null;
}
function sanitizeIdent(name) {
  let s = name.trim().toUpperCase().replace(/[^A-Z0-9]+/g, "_").replace(/^_+|_+$/g, "");
  if (!s) s = "ELEMENT";
  if (/^[0-9]/.test(s)) s = "E_" + s;
  return s;
}
function identToDisplay(ident) { return ident.toLowerCase().replace(/_/g, " "); }

function parseCellTable(text, name, count) {
  const re = new RegExp(name.replace(/[.*+?^${}()|[\]\\]/g, "\\$&") + "\\[" + count + "\\]\\[2\\]\\s*=\\s*\\{([\\s\\S]*?)\\};");
  const m = text.match(re);
  if (!m) throw new Error("Could not find cell table " + name + " in patterns.h");
  const pairs = [];
  const pairRe = /\{\s*(-?\d+)\s*,\s*(-?\d+)\s*\}/g;
  let pm;
  while ((pm = pairRe.exec(m[1]))) pairs.push([Number(pm[1]), Number(pm[2])]);
  if (pairs.length !== count) throw new Error(name + ": expected " + count + " cells, found " + pairs.length);
  return pairs;
}

function bytesToPairs(byteVals, cells) {
  const out = [];
  cells.forEach(function (c, i) {
    const byteI = i >> 3, bitI = i & 7;
    if (byteI < byteVals.length && ((byteVals[byteI] >> bitI) & 1)) out.push(c);
  });
  return out;
}
function pairsToBytes(pairs, cells) {
  const nBytes = Math.ceil(cells.length / 8);
  const bytes = new Array(nBytes).fill(0);
  const set = new Set(pairs.map(function (p) { return p[0] + "," + p[1]; }));
  cells.forEach(function (c, i) {
    if (set.has(c[0] + "," + c[1])) bytes[i >> 3] |= (1 << (i & 7));
  });
  return bytes;
}
function bytesToHexList(bytes) {
  return bytes.map(function (b) { return "0x" + b.toString(16).toUpperCase().padStart(2, "0"); }).join(", ");
}

function parseByteDecls(text, namePattern) {
  const re = new RegExp("const\\s+uint8_t\\s+(" + namePattern + ")\\s*\\[\\s*(\\d+)\\s*\\]\\s*=\\s*\\{([^}]*)\\};", "g");
  const out = {};
  let m;
  while ((m = re.exec(text))) {
    out[m[1]] = m[3].split(",").map(function (s) { return s.trim(); }).filter(Boolean)
      .map(function (s) { return parseInt(s, 16); });
  }
  return out;
}

function readGlyphs() {
  const text = fs.readFileSync(PATTERNS_H, "utf8");
  const cellTables = {
    xs: parseCellTable(text, "kHexCellQR_XS", 61),
    md: parseCellTable(text, "kHexCellQR_MD", 127),
    lg: parseCellTable(text, "kHexCellQR_LG", 271),
  };
  const fontBytes = parseByteDecls(text, "kFont_(?:XS|MD|LG)_\\w+");
  const elementBytes = parseByteDecls(text, "kElement_\\w+");

  const font = { xs: {}, md: {}, lg: {} };
  Object.keys(fontBytes).forEach(function (name) {
    const m = name.match(/^kFont_(XS|MD|LG)_(\w+)$/);
    if (!m) return;
    const sk = m[1].toLowerCase();
    const ch = identToChar(m[2]);
    if (ch == null) return;
    font[sk][ch] = bytesToPairs(fontBytes[name], cellTables[sk]);
  });

  const elements = {};
  Object.keys(elementBytes).forEach(function (name) {
    const m = name.match(/^kElement_(\w+)$/);
    if (!m) return;
    elements[identToDisplay(m[1])] = bytesToPairs(elementBytes[name], cellTables.lg);
  });

  return { cellTables: cellTables, font: font, elements: elements };
}

function writeGlyphs(font, elements) {
  let text = fs.readFileSync(PATTERNS_H, "utf8");
  const cellTables = {
    xs: parseCellTable(text, "kHexCellQR_XS", 61),
    md: parseCellTable(text, "kHexCellQR_MD", 127),
    lg: parseCellTable(text, "kHexCellQR_LG", 271),
  };
  const existingFont = parseByteDecls(text, "kFont_(?:XS|MD|LG)_\\w+");
  const existingElements = parseByteDecls(text, "kElement_\\w+");

  function declLine(name, bytes, comment) {
    return "const uint8_t " + name + "[" + bytes.length + "] = { " + bytesToHexList(bytes) + " }; // " + comment;
  }
  function replaceOrQueueInsert(name, line, insertQueue, afterAnchorRe) {
    const declRe = new RegExp("const\\s+uint8_t\\s+" + name + "\\s*\\[\\s*\\d+\\s*\\]\\s*=\\s*\\{[^}]*\\};(?:[^\\n]*)?");
    if (declRe.test(text)) {
      text = text.replace(declRe, line);
    } else {
      insertQueue.push({ name: name, line: line, afterAnchorRe: afterAnchorRe });
    }
  }

  const newInserts = { xs: [], md: [], lg: [], elements: [] };

  ["xs", "md", "lg"].forEach(function (sk) {
    Object.keys(font[sk] || {}).forEach(function (ch) {
      const name = "kFont_" + sk.toUpperCase() + "_" + identForChar(ch);
      const bytes = pairsToBytes(font[sk][ch], cellTables[sk]);
      const label = ch === " " ? "SPACE" : "'" + ch + "'";
      const line = declLine(name, bytes, label);
      const existing = existingFont[name];
      const changed = !existing || existing.length !== bytes.length || existing.some(function (b, i) { return b !== bytes[i]; });
      if (changed) replaceOrQueueInsert(name, line, newInserts[sk]);
    });
  });
  Object.keys(elements || {}).forEach(function (name0) {
    const name = "kElement_" + sanitizeIdent(name0);
    const bytes = pairsToBytes(elements[name0], cellTables.lg);
    const line = declLine(name, bytes, name0);
    const existing = existingElements[name];
    const changed = !existing || existing.length !== bytes.length || existing.some(function (b, i) { return b !== bytes[i]; });
    if (changed) replaceOrQueueInsert(name, line, newInserts.elements);
  });

  // insert brand-new glyphs/elements right after the last existing declaration of the same kind
  function insertAfterLast(matchRe, lines) {
    if (!lines.length) return;
    let lastEnd = -1, m, re = new RegExp(matchRe.source, "g");
    while ((m = re.exec(text))) lastEnd = m.index + m[0].length;
    if (lastEnd === -1) throw new Error("Could not find an anchor to insert new declarations near (pattern: " + matchRe + ")");
    const block = "\n" + lines.map(function (l) { return l.line; }).join("\n");
    text = text.slice(0, lastEnd) + block + text.slice(lastEnd);
  }
  insertAfterLast(/const\s+uint8_t\s+kFont_XS_\w+\s*\[\s*\d+\s*\]\s*=\s*\{[^}]*\};[^\n]*/, newInserts.xs);
  insertAfterLast(/const\s+uint8_t\s+kFont_MD_\w+\s*\[\s*\d+\s*\]\s*=\s*\{[^}]*\};[^\n]*/, newInserts.md);
  insertAfterLast(/const\s+uint8_t\s+kFont_LG_\w+\s*\[\s*\d+\s*\]\s*=\s*\{[^}]*\};[^\n]*/, newInserts.lg);
  insertAfterLast(/const\s+uint8_t\s+kElement_\w+\s*\[\s*\d+\s*\]\s*=\s*\{[^}]*\};[^\n]*/, newInserts.elements);

  fs.writeFileSync(PATTERNS_H, text, "utf8");
}

/* ---------------- @tunable config scanner ---------------- */

const TUNABLE_RE = /=\s*(-?\d+(?:\.\d+)?)\s*;.*@tunable\(\s*"([^"]+)"\s*,\s*(-?\d+(?:\.\d+)?)\s*,\s*(-?\d+(?:\.\d+)?)\s*(?:,\s*"([^"]+)"\s*)?\)/;

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
        pattern: m[5] || null,
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

/* ---------------- @tunable_text (string constants) ---------------- */

const TUNABLE_TEXT_RE = /=\s*"([^"]*)"\s*;.*@tunable_text\(\s*"([^"]+)"\s*\)/;

function scanTextTunables() {
  const out = [];
  TUNABLE_FILES.forEach(function (filePath) {
    if (!fs.existsSync(filePath)) return;
    const text = fs.readFileSync(filePath, "utf8");
    const lines = text.split(/\r\n|\n/);
    lines.forEach(function (line, i) {
      const m = line.match(TUNABLE_TEXT_RE);
      if (!m) return;
      out.push({
        id: path.relative(PROJECT_ROOT, filePath) + ":" + (i + 1),
        file: path.relative(PROJECT_ROOT, filePath),
        line: i + 1,
        label: m[2],
        value: m[1],
      });
    });
  });
  return out;
}

function writeTextTunable(id, value) {
  const parts = id.split(":");
  const lineNum = Number(parts.pop());
  const relFile = parts.join(":");
  const filePath = path.resolve(PROJECT_ROOT, relFile);
  if (!TUNABLE_FILES.includes(filePath)) throw new Error("Refusing to write to a file outside the tunable allowlist: " + relFile);
  if (typeof value !== "string") throw new Error("Invalid text tunable value (must be a string): " + value);
  if (/[\n\r"\\]/.test(value)) throw new Error("Text value can't contain quotes, backslashes, or newlines: " + JSON.stringify(value));

  const text = fs.readFileSync(filePath, "utf8");
  const eol = text.indexOf("\r\n") !== -1 ? "\r\n" : "\n";
  const lines = text.split(/\r\n|\n/);
  const idx = lineNum - 1;
  if (idx < 0 || idx >= lines.length) throw new Error("Tunable line number out of range: " + id);
  const line = lines[idx];
  const m = line.match(TUNABLE_TEXT_RE);
  if (!m) throw new Error("Line " + lineNum + " in " + relFile + " no longer matches the expected @tunable_text pattern -- refusing to write (did the file change?).");

  const newLine = line.replace(/=\s*"[^"]*"\s*;/, '= "' + value + '";');
  lines[idx] = newLine;
  fs.writeFileSync(filePath, lines.join(eol), "utf8");
}

/* ---------------- @tunable_enum (labeled integer choices) ---------------- */

const TUNABLE_ENUM_RE = /=\s*(-?\d+)\s*;.*@tunable_enum\(\s*"([^"]+)"\s*((?:,\s*"[^"]*"\s*)+)\)/;

function scanEnumTunables() {
  const out = [];
  TUNABLE_FILES.forEach(function (filePath) {
    if (!fs.existsSync(filePath)) return;
    const text = fs.readFileSync(filePath, "utf8");
    const lines = text.split(/\r\n|\n/);
    lines.forEach(function (line, i) {
      const m = line.match(TUNABLE_ENUM_RE);
      if (!m) return;
      const options = [];
      const optRe = /"([^"]*)"/g;
      let om;
      while ((om = optRe.exec(m[3]))) options.push(om[1]);
      out.push({
        id: path.relative(PROJECT_ROOT, filePath) + ":" + (i + 1),
        file: path.relative(PROJECT_ROOT, filePath),
        line: i + 1,
        label: m[2],
        value: Number(m[1]),
        options: options,
      });
    });
  });
  return out;
}

function writeEnumTunable(id, value) {
  const parts = id.split(":");
  const lineNum = Number(parts.pop());
  const relFile = parts.join(":");
  const filePath = path.resolve(PROJECT_ROOT, relFile);
  if (!TUNABLE_FILES.includes(filePath)) throw new Error("Refusing to write to a file outside the tunable allowlist: " + relFile);
  if (!Number.isInteger(value) || value < 0) throw new Error("Invalid enum tunable value (must be a non-negative integer index): " + value);

  const text = fs.readFileSync(filePath, "utf8");
  const eol = text.indexOf("\r\n") !== -1 ? "\r\n" : "\n";
  const lines = text.split(/\r\n|\n/);
  const idx = lineNum - 1;
  if (idx < 0 || idx >= lines.length) throw new Error("Tunable line number out of range: " + id);
  const line = lines[idx];
  const m = line.match(TUNABLE_ENUM_RE);
  if (!m) throw new Error("Line " + lineNum + " in " + relFile + " no longer matches the expected @tunable_enum pattern -- refusing to write (did the file change?).");

  const newLine = line.replace(/=\s*-?\d+\s*;/, "= " + value + ";");
  lines[idx] = newLine;
  fs.writeFileSync(filePath, lines.join(eol), "utf8");
}

/* ---------------- @thumbnail (real per-pattern colors) ---------------- */

function scanThumbnails() {
  const text = fs.readFileSync(PATTERNS_H, "utf8");
  const out = {};
  const re = /class\s+(\w+)\s*:[^\n{]*\{\s*\/\/\s*@thumbnail\(([^)]*)\)/g;
  let m;
  while ((m = re.exec(text))) {
    const colors = [];
    const colorRe = /"(#[0-9A-Fa-f]{6})"/g;
    let cm;
    while ((cm = colorRe.exec(m[2]))) colors.push(cm[1]);
    if (colors.length) out[m[1]] = colors;
  }
  return out;
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

const MIME = {
  ".html": "text/html; charset=utf-8",
  ".js": "application/javascript; charset=utf-8",
  ".css": "text/css; charset=utf-8",
};

const server = http.createServer(function (req, res) {
  const url = new URL(req.url, "http://localhost");

  try {
    if (req.method === "GET" && url.pathname === "/api/patterns") {
      return sendJson(res, 200, { patterns: parsePatterns(), thumbnails: scanThumbnails() });
    }
    if (req.method === "POST" && url.pathname === "/api/patterns") {
      return readBody(req).then(function (body) {
        writePatterns(body.patterns);
        sendJson(res, 200, { ok: true, patterns: parsePatterns(), thumbnails: scanThumbnails() });
      }).catch(function (e) { sendJson(res, 400, { error: e.message }); });
    }
    if (req.method === "GET" && url.pathname === "/api/config") {
      return sendJson(res, 200, {
        tunables: scanTunables(),
        textTunables: scanTextTunables(),
        enumTunables: scanEnumTunables(),
      });
    }
    if (req.method === "POST" && url.pathname === "/api/config") {
      return readBody(req).then(function (body) {
        (body.updates || []).forEach(function (u) { writeTunable(u.id, Number(u.value)); });
        (body.textUpdates || []).forEach(function (u) { writeTextTunable(u.id, String(u.value)); });
        (body.enumUpdates || []).forEach(function (u) { writeEnumTunable(u.id, Number(u.value)); });
        sendJson(res, 200, {
          ok: true,
          tunables: scanTunables(),
          textTunables: scanTextTunables(),
          enumTunables: scanEnumTunables(),
        });
      }).catch(function (e) { sendJson(res, 400, { error: e.message }); });
    }
    if (req.method === "GET" && url.pathname === "/api/glyphs") {
      return sendJson(res, 200, readGlyphs());
    }
    if (req.method === "POST" && url.pathname === "/api/glyphs") {
      return readBody(req).then(function (body) {
        writeGlyphs(body.font, body.elements);
        sendJson(res, 200, { ok: true, glyphs: readGlyphs() });
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

if (require.main === module) {
  server.listen(PORT, function () {
    console.log("Motionhexa console running at http://localhost:" + PORT);
    console.log("Project root: " + PROJECT_ROOT);
  });
}

module.exports = {
  readGlyphs, writeGlyphs, parsePatterns, writePatterns,
  scanTunables, writeTunable, scanTextTunables, writeTextTunable, scanEnumTunables, writeEnumTunable,
  scanThumbnails, PATTERNS_H, MAIN_CPP,
};
