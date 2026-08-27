"use strict";

var state = { patterns: [], tunables: [], dirty: false, openSettingsFor: null };

var patternListEl = document.getElementById("patternList");
var configListEl = document.getElementById("configList");
var dirtyDot = document.getElementById("dirtyDot");
var logOut = document.getElementById("logOut");
var toastEl = document.getElementById("toast");

function toast(msg, kind) {
  toastEl.textContent = msg;
  toastEl.className = "toast show" + (kind ? " " + kind : "");
  clearTimeout(toastEl._t);
  toastEl._t = setTimeout(function () { toastEl.className = "toast"; }, 3200);
}

function setDirty(v) {
  state.dirty = v;
  dirtyDot.classList.toggle("show", v);
}

/* ---------------- deterministic abstract thumbnail per effect ---------------- */

function hashStr(s) {
  var h = 0;
  for (var i = 0; i < s.length; i++) h = (h * 31 + s.charCodeAt(i)) | 0;
  return Math.abs(h);
}

function thumbnailSvg(name) {
  var h = hashStr(name);
  var hue1 = h % 360;
  var hue2 = (hue1 + 130 + ((h >> 6) % 100)) % 360;
  var c1 = "hsl(" + hue1 + " 70% 55%)";
  var c2 = "hsl(" + hue2 + " 70% 55%)";
  var motif = (h >> 3) % 5;
  var hexPoints = "15,1 28,8 28,22 15,29 2,22 2,8";
  var inner = "";
  if (motif === 0) { // scattered dots
    var dots = [];
    for (var i = 0; i < 6; i++) {
      var a = (h >> (i * 3)) % 360 * Math.PI / 180;
      var r = 4 + ((h >> (i * 2)) % 8);
      var x = 15 + Math.cos(a) * r, y = 15 + Math.sin(a) * r;
      dots.push('<circle cx="' + x.toFixed(1) + '" cy="' + y.toFixed(1) + '" r="2.2" fill="' + (i % 2 ? c1 : c2) + '"/>');
    }
    inner = dots.join("");
  } else if (motif === 1) { // concentric rings
    inner = '<circle cx="15" cy="15" r="10" fill="none" stroke="' + c1 + '" stroke-width="2"/>' +
      '<circle cx="15" cy="15" r="4.5" fill="' + c2 + '"/>';
  } else if (motif === 2) { // triangle
    inner = '<polygon points="15,6 24,22 6,22" fill="' + c1 + '" opacity="0.85"/>' +
      '<circle cx="15" cy="18" r="2.5" fill="' + c2 + '"/>';
  } else if (motif === 3) { // diagonal stripes
    inner = '<line x1="4" y1="24" x2="14" y2="4" stroke="' + c1 + '" stroke-width="3"/>' +
      '<line x1="14" y1="26" x2="24" y2="6" stroke="' + c2 + '" stroke-width="3"/>';
  } else { // glow circle
    inner = '<circle cx="15" cy="15" r="9" fill="' + c1 + '"/>' +
      '<circle cx="12" cy="12" r="3" fill="' + c2 + '" opacity="0.9"/>';
  }
  return '<svg viewBox="0 0 30 30" xmlns="http://www.w3.org/2000/svg">' +
    '<polygon points="' + hexPoints + '" fill="#191d27" stroke="#262b38"/>' +
    '<clipPath id="clip-' + h + '"><polygon points="' + hexPoints + '"/></clipPath>' +
    '<g clip-path="url(#clip-' + h + ')">' + inner + '</g>' +
    '</svg>';
}

/* ---------------- patterns: drag-and-drop, settings ---------------- */

function displayOrder() {
  // enabled patterns keep their relative order first, disabled ones sink to
  // the bottom in their own relative order -- since we never mutate the
  // underlying array's order on toggle (only the `enabled` flag), a
  // re-enabled pattern falls back into its original slot automatically.
  var enabled = state.patterns.filter(function (p) { return p.enabled; });
  var disabled = state.patterns.filter(function (p) { return !p.enabled; });
  return enabled.concat(disabled);
}

function tunablesFor(patternName) {
  return state.tunables.filter(function (t) { return t.pattern === patternName; });
}

function renderConfigInto(container, tunables, onChange) {
  container.innerHTML = "";
  if (!tunables.length) {
    container.innerHTML = '<div class="config-empty">No adjustable settings yet.</div>';
    return;
  }
  tunables.forEach(function (t) {
    var row = document.createElement("div");
    row.className = "config-row";

    var labelRow = document.createElement("div");
    labelRow.className = "label-row";
    var label = document.createElement("span");
    label.className = "label";
    label.textContent = t.label;
    var file = document.createElement("span");
    file.className = "file";
    file.textContent = t.file + ":" + t.line;
    labelRow.appendChild(label);
    labelRow.appendChild(file);

    var controlRow = document.createElement("div");
    controlRow.className = "control-row";
    var slider = document.createElement("input");
    slider.type = "range";
    slider.min = t.min; slider.max = t.max; slider.value = t.value;
    var num = document.createElement("input");
    num.type = "number";
    num.min = t.min; num.max = t.max; num.value = t.value;

    function sync(newVal) {
      newVal = Math.max(t.min, Math.min(t.max, Number(newVal)));
      t.value = newVal;
      slider.value = newVal;
      num.value = newVal;
      onChange();
    }
    slider.addEventListener("input", function () { sync(slider.value); });
    num.addEventListener("change", function () { sync(num.value); });

    controlRow.appendChild(slider);
    controlRow.appendChild(num);
    row.appendChild(labelRow);
    row.appendChild(controlRow);
    container.appendChild(row);
  });
}

var dragSrcName = null;

function renderPatterns() {
  patternListEl.innerHTML = "";
  var order = displayOrder();

  order.forEach(function (p) {
    var li = document.createElement("li");
    li.className = "pattern-row" + (p.enabled ? "" : " disabled");
    li.draggable = p.enabled;
    li.dataset.name = p.name;

    var grip = document.createElement("span");
    grip.className = "grip";
    grip.textContent = p.enabled ? "⠿" : "";

    var thumb = document.createElement("span");
    thumb.className = "thumb";
    thumb.innerHTML = thumbnailSvg(p.name);

    var cb = document.createElement("input");
    cb.type = "checkbox";
    cb.checked = p.enabled;
    cb.addEventListener("change", function () {
      p.enabled = cb.checked;
      setDirty(true);
      renderPatterns();
    });

    var name = document.createElement("span");
    name.className = "name";
    name.textContent = p.name;

    var myTunables = tunablesFor(p.name);
    var settingsBtn = document.createElement("button");
    settingsBtn.className = "settings-btn" + (myTunables.length ? " has-settings" : "");
    settingsBtn.textContent = "⚙";
    settingsBtn.title = myTunables.length ? "Settings" : "No adjustable settings for this effect";
    settingsBtn.disabled = !myTunables.length;
    settingsBtn.addEventListener("click", function () {
      state.openSettingsFor = state.openSettingsFor === p.name ? null : p.name;
      renderPatterns();
    });

    li.appendChild(grip);
    li.appendChild(thumb);
    li.appendChild(cb);
    li.appendChild(name);
    li.appendChild(settingsBtn);
    patternListEl.appendChild(li);

    if (state.openSettingsFor === p.name && myTunables.length) {
      var settingsDiv = document.createElement("div");
      settingsDiv.className = "effect-settings";
      renderConfigInto(settingsDiv, myTunables, function () { setDirty(true); });
      patternListEl.appendChild(settingsDiv);
    }

    if (p.enabled) {
      li.addEventListener("dragstart", function (e) {
        dragSrcName = p.name;
        li.classList.add("dragging");
        e.dataTransfer.effectAllowed = "move";
      });
      li.addEventListener("dragend", function () { li.classList.remove("dragging"); });
      li.addEventListener("dragover", function (e) {
        if (!dragSrcName || dragSrcName === p.name) return;
        e.preventDefault();
        li.classList.add("drag-over");
      });
      li.addEventListener("dragleave", function () { li.classList.remove("drag-over"); });
      li.addEventListener("drop", function (e) {
        e.preventDefault();
        li.classList.remove("drag-over");
        if (!dragSrcName || dragSrcName === p.name) return;
        reorderByName(dragSrcName, p.name);
        dragSrcName = null;
      });
    }
  });
}

function reorderByName(srcName, targetName) {
  // reorder within the full underlying array (both enabled and disabled),
  // so relative order stays consistent for the disabled-sink-to-bottom rule
  var srcIdx = state.patterns.findIndex(function (p) { return p.name === srcName; });
  var tgtIdx = state.patterns.findIndex(function (p) { return p.name === targetName; });
  if (srcIdx === -1 || tgtIdx === -1) return;
  var moved = state.patterns.splice(srcIdx, 1)[0];
  var newTgtIdx = state.patterns.findIndex(function (p) { return p.name === targetName; });
  state.patterns.splice(newTgtIdx, 0, moved);
  setDirty(true);
  renderPatterns();
}

/* ---------------- global config ---------------- */

function renderConfig() {
  var globals = state.tunables.filter(function (t) { return !t.pattern; });
  renderConfigInto(configListEl, globals, function () { setDirty(true); });
}

/* ---------------- load / save ---------------- */

function loadAll() {
  return Promise.all([
    fetch("/api/patterns").then(function (r) { return r.json(); }),
    fetch("/api/config").then(function (r) { return r.json(); }),
  ]).then(function (results) {
    state.patterns = results[0].patterns;
    state.tunables = results[1].tunables;
    renderPatterns();
    renderConfig();
    setDirty(false);
  }).catch(function (e) { toast("Load failed: " + e.message, "err"); });
}

document.getElementById("reloadBtn").addEventListener("click", function () {
  if (state.dirty && !confirm("Discard unsaved changes and reload from disk?")) return;
  loadAll();
});

document.getElementById("saveBtn").addEventListener("click", function () {
  var patternsPayload = { patterns: state.patterns.map(function (p) { return { name: p.name, enabled: p.enabled }; }) };
  var configPayload = { updates: state.tunables.map(function (t) { return { id: t.id, value: t.value }; }) };

  Promise.all([
    fetch("/api/patterns", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(patternsPayload) })
      .then(function (r) { return r.json(); }).then(function (j) { if (j.error) throw new Error(j.error); }),
    fetch("/api/config", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(configPayload) })
      .then(function (r) { return r.json(); }).then(function (j) { if (j.error) throw new Error(j.error); }),
  ]).then(function () {
    setDirty(false);
    toast("Saved to disk. Build or Deploy to apply.", "ok");
  }).catch(function (e) { toast("Save failed: " + e.message, "err"); });
});

/* ---------------- build / deploy (streamed) ---------------- */

function streamAction(url, label) {
  logOut.textContent = "";
  var btns = [document.getElementById("buildBtn"), document.getElementById("deployBtn")];
  btns.forEach(function (b) { b.disabled = true; });

  fetch(url, { method: "POST" }).then(function (res) {
    var reader = res.body.getReader();
    var decoder = new TextDecoder();
    var buf = "";

    function pump() {
      return reader.read().then(function (result) {
        if (result.done) return;
        buf += decoder.decode(result.value, { stream: true });
        var chunks = buf.split("\n\n");
        buf = chunks.pop();
        chunks.forEach(function (chunk) {
          var eventMatch = chunk.match(/^event: (\w+)\ndata: (.*)$/s);
          if (!eventMatch) return;
          var event = eventMatch[1];
          var data = JSON.parse(eventMatch[2]);
          if (event === "log") {
            logOut.textContent += data;
            logOut.scrollTop = logOut.scrollHeight;
          } else if (event === "done") {
            btns.forEach(function (b) { b.disabled = false; });
            toast(label + (data.code === 0 ? " succeeded." : " failed (exit " + data.code + ")."), data.code === 0 ? "ok" : "err");
          }
        });
        return pump();
      });
    }
    return pump();
  }).catch(function (e) {
    logOut.textContent += "\n[connection error] " + e.message;
    btns.forEach(function (b) { b.disabled = false; });
  });
}

document.getElementById("buildBtn").addEventListener("click", function () {
  streamAction("/api/build", "Build");
});
document.getElementById("deployBtn").addEventListener("click", function () {
  if (!confirm("This will build and flash the physical device over USB. Continue?")) return;
  streamAction("/api/deploy", "Deploy");
});

loadAll();
