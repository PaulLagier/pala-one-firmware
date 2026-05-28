// Tiny auto-escaping HTML templating helper. No dependencies.
//
// Usage:
//   var html = window.palaHtml.html, raw = window.palaHtml.raw;
//   el.innerHTML = html`<p class="meta">${userText}</p>`;
//
// Every ${value} interpolated into html`` is HTML-escaped automatically, so
// the common XSS footgun (forgetting to escape one field) cannot happen.
// Escaping is keyed to the ${} boundary — it does NOT parse HTML, so never
// build a tag name or attribute *name* from untrusted data.
//
// resolve() rules for an interpolated value:
//   - null / undefined / false -> ""   (so `${cond && html`...`}` works)
//   - Array                    -> each item resolved + joined  (lists)
//   - Raw (from html`` or raw())-> inserted verbatim, NOT escaped (nesting)
//   - anything else            -> String()'d and escaped
(function () {
  function escapeHtml(s) {
    return String(s == null ? "" : s)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;")
      .replace(/'/g, "&#39;");
  }

  // Wrapper marking a string as trusted, pre-built HTML. toString() lets a
  // Raw be assigned straight to innerHTML (the setter coerces to string).
  function Raw(value) { this.value = String(value); }
  Raw.prototype.toString = function () { return this.value; };

  function raw(value) { return new Raw(value); }

  function resolve(v) {
    if (v == null || v === false) return "";
    if (v instanceof Raw) return v.value;
    if (Array.isArray(v)) return v.map(resolve).join("");
    return escapeHtml(v);
  }

  function html(strings) {
    var values = Array.prototype.slice.call(arguments, 1);
    var out = strings[0];
    for (var i = 0; i < values.length; i++) {
      out += resolve(values[i]) + strings[i + 1];
    }
    return new Raw(out);
  }

  window.palaHtml = { html: html, raw: raw, escapeHtml: escapeHtml };
})();
