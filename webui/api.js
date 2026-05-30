// Tiny fetch wrapper for the SPA. Every /api/* endpoint returns JSON, so
// callers can assume `.get()` and `.post()` resolve with parsed objects and
// reject with an Error on non-2xx. Network errors propagate as-is.
//
// Server errors (4xx/5xx) carry the response text as `err.message`. Callers
// that want the status code can read `err.status`.

(function () {
  function build(opts) {
    opts = opts || {};
    var init = { method: opts.method || "GET" };
    if (opts.body !== undefined) {
      init.headers = { "Content-Type": "application/json" };
      init.body    = JSON.stringify(opts.body);
    }
    return init;
  }

  async function request(path, opts) {
    var r = await fetch(path, build(opts));
    if (!r.ok) {
      var text = "";
      try { text = await r.text(); } catch (_) { /* ignore */ }
      var err = new Error(text || (r.status + " " + r.statusText));
      err.status = r.status;
      throw err;
    }
    // Empty body is fine for POSTs that don't need to return anything —
    // expose it as null rather than crashing on JSON.parse("").
    var ct = r.headers.get("Content-Type") || "";
    if (!ct.includes("application/json")) return null;
    var body = await r.text();
    return body ? JSON.parse(body) : null;
  }

  // Multipart upload helper. Uses XHR (not fetch) so we can wire
  // upload-progress events for a UI indicator. Resolves with the response
  // text on 2xx; rejects with an Error whose .message is the response body
  // (typically a useful plain-text reason) on 4xx/5xx.
  function upload(path, formData, onProgress) {
    return new Promise(function (resolve, reject) {
      var xhr = new XMLHttpRequest();
      xhr.open("POST", path);
      if (onProgress) {
        xhr.upload.addEventListener("progress", function (ev) {
          if (ev.lengthComputable) onProgress(ev.loaded / ev.total, ev.loaded, ev.total);
        });
      }
      xhr.onload = function () {
        if (xhr.status >= 200 && xhr.status < 300) {
          resolve(xhr.responseText || "");
        } else {
          var err = new Error(xhr.responseText || (xhr.status + " " + xhr.statusText));
          err.status = xhr.status;
          reject(err);
        }
      };
      xhr.onerror = function () { reject(new Error("network error")); };
      xhr.send(formData);
    });
  }

  window.palaApi = {
    get:    function (path)       { return request(path, { method: "GET" }); },
    post:   function (path, body) { return request(path, { method: "POST", body: body }); },
    upload: upload
  };
})();
