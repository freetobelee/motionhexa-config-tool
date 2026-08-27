"use strict";

var state = { tunables: [], textTunables: [], enumTunables: [], dirty: false };

var dirtyDot = document.getElementById("dirtyDot");
var nameConfigEl = document.getElementById("nameConfig");
var brightnessConfigEl = document.getElementById("brightnessConfig");
var chargeStyleConfigEl = document.getElementById("chargeStyleConfig");

function setDirty(v) {
  state.dirty = v;
  dirtyDot.classList.toggle("show", v);
}

function renderTextField(container, tunable) {
  container.innerHTML = "";
  var row = document.createElement("div");
  row.className = "config-row";
  var labelRow = document.createElement("div");
  labelRow.className = "label-row";
  var label = document.createElement("span");
  label.className = "label";
  label.textContent = tunable.label;
  var file = document.createElement("span");
  file.className = "file";
  file.textContent = tunable.file + ":" + tunable.line;
  labelRow.appendChild(label);
  labelRow.appendChild(file);

  var input = document.createElement("input");
  input.type = "text";
  input.value = tunable.value;
  input.maxLength = 60;
  input.addEventListener("input", function () {
    tunable.value = input.value;
    setDirty(true);
  });

  row.appendChild(labelRow);
  row.appendChild(input);
  container.appendChild(row);
}

function renderSlider(container, tunable) {
  container.innerHTML = "";
  var row = document.createElement("div");
  row.className = "config-row";
  var labelRow = document.createElement("div");
  labelRow.className = "label-row";
  var label = document.createElement("span");
  label.className = "label";
  label.textContent = tunable.label;
  var file = document.createElement("span");
  file.className = "file";
  file.textContent = tunable.file + ":" + tunable.line;
  labelRow.appendChild(label);
  labelRow.appendChild(file);

  var controlRow = document.createElement("div");
  controlRow.className = "control-row";
  var slider = document.createElement("input");
  slider.type = "range";
  slider.min = tunable.min; slider.max = tunable.max; slider.value = tunable.value;
  var num = document.createElement("input");
  num.type = "number";
  num.min = tunable.min; num.max = tunable.max; num.value = tunable.value;

  function sync(newVal) {
    newVal = Math.max(tunable.min, Math.min(tunable.max, Number(newVal)));
    tunable.value = newVal;
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
  container.appendChild(row);
}

function renderEnum(container, tunable) {
  container.innerHTML = "";
  var row = document.createElement("div");
  row.className = "config-row";
  var labelRow = document.createElement("div");
  labelRow.className = "label-row";
  var label = document.createElement("span");
  label.className = "label";
  label.textContent = tunable.label;
  var file = document.createElement("span");
  file.className = "file";
  file.textContent = tunable.file + ":" + tunable.line;
  labelRow.appendChild(label);
  labelRow.appendChild(file);

  var optionsRow = document.createElement("div");
  optionsRow.className = "enum-options";
  tunable.options.forEach(function (optLabel, i) {
    var btn = document.createElement("button");
    btn.className = "enum-option" + (tunable.value === i ? " selected" : "");
    btn.textContent = optLabel;
    btn.addEventListener("click", function () {
      tunable.value = i;
      setDirty(true);
      renderEnum(container, tunable);
    });
    optionsRow.appendChild(btn);
  });

  row.appendChild(labelRow);
  row.appendChild(optionsRow);
  container.appendChild(row);
}

function loadAll() {
  return fetch("/api/config").then(function (r) { return r.json(); }).then(function (result) {
    state.tunables = result.tunables;
    state.textTunables = result.textTunables;
    state.enumTunables = result.enumTunables;

    var nameTunable = state.textTunables.find(function (t) { return t.label === "Device Name"; });
    if (nameTunable) renderTextField(nameConfigEl, nameTunable);
    else nameConfigEl.innerHTML = '<div class="config-empty">Not found.</div>';

    var brightnessTunable = state.tunables.find(function (t) { return t.label === "Default Brightness"; });
    if (brightnessTunable) renderSlider(brightnessConfigEl, brightnessTunable);
    else brightnessConfigEl.innerHTML = '<div class="config-empty">Not found.</div>';

    var chargeTunable = state.enumTunables.find(function (t) { return t.label === "Charge Indicator"; });
    if (chargeTunable) renderEnum(chargeStyleConfigEl, chargeTunable);
    else chargeStyleConfigEl.innerHTML = '<div class="config-empty">Not found.</div>';

    setDirty(false);
  }).catch(function (e) { window.toast("Load failed: " + e.message, "err"); });
}

document.getElementById("reloadBtn").addEventListener("click", function () {
  if (state.dirty && !confirm("Discard unsaved changes and reload from disk?")) return;
  loadAll();
});

document.getElementById("saveBtn").addEventListener("click", function () {
  var payload = {
    updates: state.tunables.map(function (t) { return { id: t.id, value: t.value }; }),
    textUpdates: state.textTunables.map(function (t) { return { id: t.id, value: t.value }; }),
    enumUpdates: state.enumTunables.map(function (t) { return { id: t.id, value: t.value }; }),
  };
  fetch("/api/config", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(payload) })
    .then(function (r) { return r.json(); })
    .then(function (j) {
      if (j.error) throw new Error(j.error);
      setDirty(false);
      window.toast("Saved to disk. Build or Deploy to apply.", "ok");
    }).catch(function (e) { window.toast("Save failed: " + e.message, "err"); });
});

loadAll();
