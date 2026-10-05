"use strict";
// Shared header Build/Deploy wiring + toast, used identically by every page
// (Programs, System, Fonts & Elements) so those controls work the same no
// matter which tab you're on.

// Every API call goes through here so a response that isn't the JSON we expect (a crashed
// server, a dropped connection) surfaces as a readable message instead of a parse error,
// and the server's own { error } payload is raised as-is.
window.fetchJson = function (url, options) {
  return fetch(url, options).then(function (res) {
    return res.text().then(function (text) {
      var json;
      try { json = JSON.parse(text); }
      catch (e) { throw new Error("server returned " + res.status + " with an unexpected response"); }
      if (json && json.error) throw new Error(json.error);
      if (!res.ok) throw new Error("server returned " + res.status);
      return json;
    });
  });
};

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

    var finished = false;
    function finish(status, msg, kind) {
      if (finished) return;
      finished = true;
      [buildBtn, deployBtn].forEach(function (b) { if (b) b.disabled = false; });
      setStatus(status);
      if (msg) window.toast(msg, kind);
    }

    fetch(url, { method: "POST" }).then(function (res) {
      if (!res.ok || !res.body) throw new Error("server returned " + res.status);
      var reader = res.body.getReader();
      var decoder = new TextDecoder();
      var buf = "";

      function pump() {
        return reader.read().then(function (result) {
          // a stream that ends without a `done` event means pio died or the connection
          // dropped; without this the buttons would stay disabled with no way to retry
          if (result.done) {
            finish(label + " ended unexpectedly", label + " ended without finishing -- check the log.", "err");
            return;
          }
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
              var ok = data.code === 0;
              finish(ok ? label + " succeeded" : label + " failed (exit " + data.code + ")",
                     label + (ok ? " succeeded." : " failed (exit " + data.code + ")."), ok ? "ok" : "err");
            }
          });
          return pump();
        });
      }
      return pump();
    }).catch(function (e) {
      if (logOut) logOut.textContent += "\n[connection error] " + e.message;
      finish(label + " failed to start", label + " failed to start: " + e.message, "err");
    });
  }

  if (buildBtn) buildBtn.addEventListener("click", function () { streamAction("/api/build", "Build"); });
  if (deployBtn) deployBtn.addEventListener("click", function () {
    if (!confirm("This will build and flash the physical device over USB. Continue?")) return;
    streamAction("/api/deploy", "Deploy");
  });
}

document.addEventListener("DOMContentLoaded", initSharedHeader);
