"use strict";

var state = { patterns: [], thumbnails: {}, instructions: {}, tunables: [], dirty: false, openSettingsFor: null };

var patternListEl = document.getElementById("patternList");
var dirtyDot = document.getElementById("dirtyDot");
var logOut = document.getElementById("logOut");

function setDirty(v) {
  state.dirty = v;
  dirtyDot.classList.toggle("show", v);
}

var DISCLOSURE_SVG = '<svg viewBox="0 0 10 6" xmlns="http://www.w3.org/2000/svg"><polygon points="0,0 10,0 5,6" fill="currentColor"/></svg>';

/* ---------------- real, per-program thumbnail (from @thumbnail colors) ---------------- */

function hashStr(s) {
  var h = 0;
  for (var i = 0; i < s.length; i++) h = (h * 31 + s.charCodeAt(i)) | 0;
  return Math.abs(h);
}

function thumbnailSvg(name) {
  var colors = state.thumbnails[name];
  var hexPoints = "23,1 43,12 43,34 23,45 3,34 3,12";
  if (!colors || !colors.length) {
    return '<svg viewBox="0 0 46 46" xmlns="http://www.w3.org/2000/svg">' +
      '<polygon points="' + hexPoints + '" fill="#191d27" stroke="#262b38"/></svg>';
  }
  var h = hashStr(name);
  var fills;
  if (colors.length === 1) {
    fills = '<rect x="0" y="0" width="46" height="46" fill="' + colors[0] + '"/>';
  } else {
    // evenly-spaced horizontal bands through the real color list -- a direct,
    // literal swatch of that program's own colors, not a generated pattern
    var bandH = 46 / colors.length;
    fills = colors.map(function (c, i) {
      return '<rect x="0" y="' + (i * bandH).toFixed(1) + '" width="46" height="' + (bandH + 0.5).toFixed(1) + '" fill="' + c + '"/>';
    }).join("");
  }
  var clipId = "clip-" + h;
  return '<svg viewBox="0 0 46 46" xmlns="http://www.w3.org/2000/svg">' +
    '<clipPath id="' + clipId + '"><polygon points="' + hexPoints + '"/></clipPath>' +
    '<g clip-path="url(#' + clipId + ')">' + fills + '</g>' +
    '<polygon points="' + hexPoints + '" fill="none" stroke="#262b38"/>' +
    '</svg>';
}

/* ---------------- patterns: drag-and-drop, disclosure settings ---------------- */

function displayOrder() {
  // enabled programs keep their relative order first, disabled ones sink to
  // the bottom in their own relative order -- since we never mutate the
  // underlying array's order on toggle (only the `enabled` flag), a
  // re-enabled program falls back into its original slot automatically.
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
    li.dataset.name = p.name;

    var grip = document.createElement("span");
    grip.className = "grip" + (p.enabled ? "" : " static");
    grip.textContent = "⠿";
    grip.title = "Drag to reorder";
    grip.draggable = p.enabled;

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
    var myInstructions = state.instructions[p.name];
    var hasPanel = myTunables.length > 0 || !!myInstructions;
    var isOpen = state.openSettingsFor === p.name;
    var disclosure = document.createElement("button");
    disclosure.className = "disclosure" + (hasPanel ? " has-settings" : "") + (isOpen ? " open" : "");
    disclosure.innerHTML = DISCLOSURE_SVG;
    disclosure.title = hasPanel ? "Settings" : "No adjustable settings for this program";
    disclosure.disabled = !hasPanel;
    disclosure.addEventListener("click", function () {
      state.openSettingsFor = isOpen ? null : p.name;
      renderPatterns();
    });

    li.appendChild(grip);
    li.appendChild(thumb);
    li.appendChild(cb);
    li.appendChild(name);
    li.appendChild(disclosure);
    patternListEl.appendChild(li);

    if (isOpen && hasPanel) {
      var settingsDiv = document.createElement("div");
      settingsDiv.className = "effect-settings";
      if (myInstructions) {
        var instructionsP = document.createElement("p");
        instructionsP.className = "instructions";
        instructionsP.textContent = myInstructions;
        settingsDiv.appendChild(instructionsP);
      }
      if (myTunables.length) {
        renderConfigInto(settingsDiv, myTunables, function () { setDirty(true); });
      }
      patternListEl.appendChild(settingsDiv);
    }

    if (p.enabled) {
      grip.addEventListener("dragstart", function (e) {
        dragSrcName = p.name;
        li.classList.add("dragging");
        e.dataTransfer.effectAllowed = "move";
      });
      grip.addEventListener("dragend", function () { li.classList.remove("dragging"); });
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

/* ---------------- load / save ---------------- */

function loadAll() {
  return Promise.all([
    fetch("/api/patterns").then(function (r) { return r.json(); }),
    fetch("/api/config").then(function (r) { return r.json(); }),
  ]).then(function (results) {
    state.patterns = results[0].patterns;
    state.thumbnails = results[0].thumbnails || {};
    state.instructions = results[0].instructions || {};
    state.tunables = results[1].tunables;
    renderPatterns();
    setDirty(false);
  }).catch(function (e) { window.toast("Load failed: " + e.message, "err"); });
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
    window.toast("Saved to disk. Build or Deploy to apply.", "ok");
  }).catch(function (e) { window.toast("Save failed: " + e.message, "err"); });
});

loadAll();
