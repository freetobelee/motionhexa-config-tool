"use strict";

var state = { patterns: [], tunables: [], dirty: false };

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

/* ---------------- patterns ---------------- */

function renderPatterns() {
  patternListEl.innerHTML = "";
  state.patterns.forEach(function (p, i) {
    var li = document.createElement("li");
    li.className = "pattern-row" + (p.enabled ? "" : " disabled");

    var cb = document.createElement("input");
    cb.type = "checkbox";
    cb.checked = p.enabled;
    cb.addEventListener("change", function () {
      p.enabled = cb.checked;
      li.classList.toggle("disabled", !p.enabled);
      setDirty(true);
    });

    var name = document.createElement("span");
    name.className = "name";
    name.textContent = p.name;

    var orderBtns = document.createElement("div");
    orderBtns.className = "order-btns";
    var up = document.createElement("button");
    up.textContent = "▲";
    up.disabled = i === 0;
    up.title = "Move up";
    up.addEventListener("click", function () {
      var tmp = state.patterns[i - 1];
      state.patterns[i - 1] = state.patterns[i];
      state.patterns[i] = tmp;
      setDirty(true);
      renderPatterns();
    });
    var down = document.createElement("button");
    down.textContent = "▼";
    down.disabled = i === state.patterns.length - 1;
    down.title = "Move down";
    down.addEventListener("click", function () {
      var tmp = state.patterns[i + 1];
      state.patterns[i + 1] = state.patterns[i];
      state.patterns[i] = tmp;
      setDirty(true);
      renderPatterns();
    });
    orderBtns.appendChild(up);
    orderBtns.appendChild(down);

    li.appendChild(cb);
    li.appendChild(name);
    li.appendChild(orderBtns);
    patternListEl.appendChild(li);
  });
}

/* ---------------- config ---------------- */

function renderConfig() {
  configListEl.innerHTML = "";
  if (!state.tunables.length) {
    configListEl.innerHTML = '<div class="config-empty">No @tunable-tagged constants found.</div>';
    return;
  }
  state.tunables.forEach(function (t) {
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
      setDirty(true);
    }
    slider.addEventListener("input", function () { sync(slider.value); });
    num.addEventListener("change", function () { sync(num.value); });

    controlRow.appendChild(slider);
    controlRow.appendChild(num);
    row.appendChild(labelRow);
    row.appendChild(controlRow);
    configListEl.appendChild(row);
  });
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
