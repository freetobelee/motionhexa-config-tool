"use strict";
// Shared header Build/Deploy wiring + toast, used identically by every page
// (Programs, System, Fonts & Elements) so those controls work the same no
// matter which tab you're on.

function initSharedHeader() {
  var toastEl = document.getElementById("toast");
  window.toast = function (msg, kind) {
    if (!toastEl) return;
    toastEl.textContent = msg;
    toastEl.className = "toast show" + (kind ? " " + kind : "");
    clearTimeout(toastEl._t);
    toastEl._t = setTimeout(function () { toastEl.className = "toast"; }, 3200);
  };

  var logOut = document.getElementById("logOut");
  var buildBtn = document.getElementById("buildBtn");
  var deployBtn = document.getElementById("deployBtn");
  var headerStatus = document.getElementById("headerBuildStatus");

  function setStatus(text) {
    if (headerStatus) headerStatus.textContent = text;
  }

  function streamAction(url, label) {
    if (logOut) logOut.textContent = "";
    setStatus(label + "...");
    [buildBtn, deployBtn].forEach(function (b) { if (b) b.disabled = true; });

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
              if (logOut) {
                logOut.textContent += data;
                logOut.scrollTop = logOut.scrollHeight;
              }
            } else if (event === "done") {
              [buildBtn, deployBtn].forEach(function (b) { if (b) b.disabled = false; });
              var ok = data.code === 0;
              setStatus(ok ? label + " succeeded" : label + " failed (exit " + data.code + ")");
              window.toast(label + (ok ? " succeeded." : " failed (exit " + data.code + ")."), ok ? "ok" : "err");
            }
          });
          return pump();
        });
      }
      return pump();
    }).catch(function (e) {
      if (logOut) logOut.textContent += "\n[connection error] " + e.message;
      [buildBtn, deployBtn].forEach(function (b) { if (b) b.disabled = false; });
      setStatus(label + " failed to start");
    });
  }

  if (buildBtn) buildBtn.addEventListener("click", function () { streamAction("/api/build", "Build"); });
  if (deployBtn) deployBtn.addEventListener("click", function () {
    if (!confirm("This will build and flash the physical device over USB. Continue?")) return;
    streamAction("/api/deploy", "Deploy");
  });
}

document.addEventListener("DOMContentLoaded", initSharedHeader);
