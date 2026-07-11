(function () {
  "use strict";

  var TICK = String.fromCharCode(96);
  var STORAGE = {
    theme: "qbt-material-theme-v1",
    imported: "qbt-material-imported-docs-v1",
    search: "qbt-material-search-state-v1"
  };
  var CATEGORY_ORDER = [
    "Overview", "Wiki", "Get started", "Interface", "Design",
    "Product", "Engineering", "Reference", "Imported"
  ];
  var FILTER_FIELDS = [
    ["all", "Every field"],
    ["title", "Title"],
    ["path", "Path"],
    ["category", "Category"],
    ["format", "File type"],
    ["content", "Content"]
  ];
  var FILTER_OPERATORS = [
    ["contains", "contains"],
    ["equals", "equals"],
    ["starts", "starts with"],
    ["ends", "ends with"],
    ["regex", "matches regex"],
    ["not-regex", "does not match regex"]
  ];

  function byId(id) { return document.getElementById(id); }
  function all(selector, root) {
    return Array.prototype.slice.call((root || document).querySelectorAll(selector));
  }
  function escapeHtml(value) {
    return String(value == null ? "" : value)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;")
      .replace(/'/g, "&#39;");
  }
  function escapeRegExp(value) {
    return String(value).replace(/[.*+?^$()|[\]\\{}]/g, "\\$&");
  }
  function slugify(value) {
    return String(value || "document").toLowerCase()
      .normalize("NFKD").replace(/[\u0300-\u036f]/g, "")
      .replace(/[^a-z0-9]+/g, "-").replace(/^-|-$/g, "") || "document";
  }
  function safeJsonParse(value, fallback) {
    if (value == null || value === "") return fallback;
    try {
      var parsed = JSON.parse(value);
      return parsed == null ? fallback : parsed;
    } catch (error) { return fallback; }
  }
  function loadStored(key, fallback) {
    try { return safeJsonParse(localStorage.getItem(key), fallback); }
    catch (error) { return fallback; }
  }
  function saveStored(key, value) {
    try {
      localStorage.setItem(key, JSON.stringify(value));
      return true;
    } catch (error) {
      showSnackbar("Browser storage is full; export your local pages before leaving.");
      return false;
    }
  }
  function debounce(fn, delay) {
    var timer = 0;
    return function () {
      var args = arguments;
      clearTimeout(timer);
      timer = setTimeout(function () { fn.apply(null, args); }, delay);
    };
  }
  function showDialog(dialog) {
    if (!dialog) return;
    if (typeof dialog.showModal === "function") dialog.showModal();
    else dialog.setAttribute("open", "");
  }
  function closeDialog(dialog) {
    if (!dialog) return;
    if (typeof dialog.close === "function") dialog.close();
    else dialog.removeAttribute("open");
  }
  function showSnackbar(message) {
    var bar = byId("snackbar");
    var label = byId("snackbar-message");
    if (!bar || !label) return;
    label.textContent = message;
    bar.hidden = false;
    bar.classList.add("is-visible");
    clearTimeout(showSnackbar.timer);
    showSnackbar.timer = setTimeout(function () {
      bar.classList.remove("is-visible");
      setTimeout(function () { bar.hidden = true; }, 220);
    }, 4200);
  }

  var corpus = window.QBT_DOCS && Array.isArray(window.QBT_DOCS.documents)
    ? window.QBT_DOCS.documents : [];
  var imported = loadStored(STORAGE.imported, []);
  if (!Array.isArray(imported)) imported = [];
  var savedSearch = loadStored(STORAGE.search, {});
  var state = {
    docs: [],
    docBySlug: new Map(),
    docByPath: new Map(),
    activeSlug: "",
    query: typeof savedSearch.query === "string" ? savedSearch.query : "",
    regex: Boolean(savedSearch.regex),
    caseSensitive: Boolean(savedSearch.caseSensitive),
    wholeWord: Boolean(savedSearch.wholeWord),
    regexFlags: typeof savedSearch.regexFlags === "string" ? savedSearch.regexFlags : "gi",
    filters: Array.isArray(savedSearch.filters) ? savedSearch.filters : [],
    filterMode: savedSearch.filterMode === "any" ? "any" : "all",
    imported: imported
  };
  var filterDraft = [];

  function normalizeDocument(doc, index, importedDoc) {
    var path = String(doc.path || ((importedDoc ? "local/" : "docs/") + (doc.title || "Document") + ".md"))
      .replace(/\\/g, "/");
    return {
      slug: slugify(doc.slug || (importedDoc ? "local-" : "") + (doc.title || path || index)),
      title: String(doc.title || path.split("/").pop().replace(/\.[^.]+$/, "") || "Document"),
      path: path,
      category: String(doc.category || (importedDoc ? "Imported" : "Reference")),
      format: String(doc.format || (/\.json$/i.test(path) ? "json" : "markdown")),
      content: String(doc.content == null ? "" : doc.content),
      imported: Boolean(importedDoc),
      importedAt: doc.importedAt || null
    };
  }

  function rebuildCorpus() {
    var result = [];
    var used = new Set();
    corpus.forEach(function (doc, index) {
      var normalized = normalizeDocument(doc, index, false);
      var base = normalized.slug;
      var suffix = 2;
      while (used.has(normalized.slug)) normalized.slug = base + "-" + suffix++;
      used.add(normalized.slug);
      result.push(normalized);
    });
    state.imported.forEach(function (doc, index) {
      var normalized = normalizeDocument(doc, index, true);
      var base = normalized.slug;
      var suffix = 2;
      while (used.has(normalized.slug)) normalized.slug = base + "-" + suffix++;
      used.add(normalized.slug);
      result.push(normalized);
    });
    state.docs = result;
    state.docBySlug = new Map();
    state.docByPath = new Map();
    result.forEach(function (doc) {
      state.docBySlug.set(doc.slug, doc);
      state.docByPath.set(doc.path.toLowerCase(), doc);
      state.docByPath.set(doc.path.split("/").pop().toLowerCase(), doc);
    });
    var count = byId("document-count");
    if (count) count.textContent = result.length + " documents";
  }

  function categoryRank(category) {
    var index = CATEGORY_ORDER.indexOf(category);
    return index < 0 ? CATEGORY_ORDER.length : index;
  }

  function sortedDocuments() {
    return state.docs.slice().sort(function (a, b) {
      return categoryRank(a.category) - categoryRank(b.category)
        || a.category.localeCompare(b.category)
        || a.title.localeCompare(b.title);
    });
  }

  function renderNavigation() {
    var nav = byId("doc-nav");
    if (!nav) return;
    var previousCategory = "";
    var html = "";
    sortedDocuments().forEach(function (doc) {
      if (doc.category !== previousCategory) {
        previousCategory = doc.category;
        html += '<div class="doc-nav-group">' + escapeHtml(previousCategory) + "</div>";
      }
      html += '<button type="button" data-doc-slug="' + escapeHtml(doc.slug) + '"'
        + (doc.slug === state.activeSlug ? ' class="is-active" aria-current="page"' : "")
        + '><svg aria-hidden="true" viewBox="0 0 24 24"><path d="M6 3h9l4 4v14H6zM14 3v5h5"/></svg>'
        + "<span>" + escapeHtml(doc.title) + "</span></button>";
    });
    nav.innerHTML = html || '<span class="toc-empty">No documents are available.</span>';
  }

  function resolveRepoPath(basePath, relativePath) {
    var base = basePath.split("/");
    base.pop();
    String(relativePath).split("/").forEach(function (part) {
      if (!part || part === ".") return;
      if (part === "..") base.pop();
      else base.push(part);
    });
    return base.join("/");
  }

  function safeExternalUrl(url) {
    var value = String(url || "").trim();
    if (/^(?:https?:|mailto:)/i.test(value)) return value;
    return "";
  }

  function resolveImageUrl(url, doc) {
    var value = String(url || "").trim();
    var external = safeExternalUrl(value);
    if (external) return external;
    if (/^(?:data:image\/(?:png|jpeg|gif|webp|svg\+xml);base64,)/i.test(value)) return value;
    value = value.replace(/^\.\//, "");
    if (doc.path.indexOf("docs/wiki/") === 0 && value.indexOf("images/") === 0) return value;
    var resolved = resolveRepoPath(doc.path, value);
    if (resolved.indexOf("docs/") === 0) return resolved.substring(5);
    if (resolved.indexOf("README") === 0) return value.replace(/^docs\//, "");
    return "https://raw.githubusercontent.com/codingmachineedge/qbittorrent-material/master/" + resolved;
  }

  function resolveDocumentLink(url, doc) {
    var value = String(url || "").trim();
    if (!value) return { href: "#", external: false };
    if (value.charAt(0) === "#") return { href: value, external: false };
    var external = safeExternalUrl(value);
    if (external) return { href: external, external: true };
    var parts = value.split("#");
    var path = parts[0];
    var anchor = parts[1] || "";
    var resolved = resolveRepoPath(doc.path, path).toLowerCase();
    var target = state.docByPath.get(resolved) || state.docByPath.get(path.toLowerCase());
    if (target) {
      return {
        href: "#wiki/" + encodeURIComponent(target.slug) + (anchor ? "/" + encodeURIComponent(anchor) : ""),
        slug: target.slug,
        anchor: anchor,
        external: false
      };
    }
    return {
      href: "https://github.com/codingmachineedge/qbittorrent-material/blob/master/" + resolveRepoPath(doc.path, path),
      external: true
    };
  }

  function inlineMarkdown(source, doc) {
    var tokens = [];
    function reserve(html) {
      var marker = "\u0001" + tokens.length + "\u0002";
      tokens.push(html);
      return marker;
    }
    var text = String(source || "");
    var codePattern = new RegExp(TICK + "([^" + TICK + "]+)" + TICK, "g");
    text = text.replace(codePattern, function (_, code) {
      return reserve("<code>" + escapeHtml(code) + "</code>");
    });
    text = text.replace(/!\[([^\]]*)\]\(([^)\s]+)(?:\s+"[^"]*")?\)/g, function (_, alt, url) {
      return reserve('<img src="' + escapeHtml(resolveImageUrl(url, doc)) + '" alt="' + escapeHtml(alt) + '" loading="lazy">');
    });
    text = text.replace(/\[([^\]]+)\]\(([^)\s]+)(?:\s+"[^"]*")?\)/g, function (_, label, url) {
      var link = resolveDocumentLink(url, doc);
      return reserve('<a href="' + escapeHtml(link.href) + '"'
        + (link.slug ? ' data-doc-slug="' + escapeHtml(link.slug) + '"' : "")
        + (link.anchor ? ' data-doc-anchor="' + escapeHtml(link.anchor) + '"' : "")
        + (link.external ? ' target="_blank" rel="noopener noreferrer"' : "")
        + ">" + escapeHtml(label) + "</a>");
    });
    text = text.replace(/<((?:https?:\/\/)[^ >]+)>/g, function (_, url) {
      return reserve('<a href="' + escapeHtml(url) + '" target="_blank" rel="noopener noreferrer">' + escapeHtml(url) + "</a>");
    });
    text = escapeHtml(text);
    text = text
      .replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>")
      .replace(/__([^_]+)__/g, "<strong>$1</strong>")
      .replace(/~~([^~]+)~~/g, "<del>$1</del>")
      .replace(/(^|[\s(])\*([^*\n]+)\*/g, "$1<em>$2</em>")
      .replace(/(^|[\s(])_([^_\n]+)_/g, "$1<em>$2</em>");
    text = text.replace(/\u0001(\d+)\u0002/g, function (_, index) {
      return tokens[Number(index)] || "";
    });
    return text;
  }

  function headingId(text, used) {
    var base = slugify(text);
    var id = base;
    var suffix = 2;
    while (used.has(id)) id = base + "-" + suffix++;
    used.add(id);
    return id;
  }

  function isTableDivider(line) {
    return /^\s*\|?\s*:?-{3,}:?\s*(?:\|\s*:?-{3,}:?\s*)+\|?\s*$/.test(line || "");
  }
  function splitTableRow(line) {
    return String(line).trim().replace(/^\||\|$/g, "").split("|").map(function (cell) {
      return cell.trim();
    });
  }
  function isBlockStart(lines, index) {
    var line = lines[index] || "";
    if (!line.trim()) return true;
    if (/^#{1,6}\s+/.test(line) || /^>\s?/.test(line) || /^\s*([-+*]|\d+\.)\s+/.test(line)) return true;
    if (/^\s*(?:---+|\*\*\*+)\s*$/.test(line)) return true;
    if (line.trim().indexOf(TICK + TICK + TICK) === 0) return true;
    if (index + 1 < lines.length && line.indexOf("|") >= 0 && isTableDivider(lines[index + 1])) return true;
    return false;
  }

  function renderMarkdown(content, doc) {
    var lines = String(content || "").replace(/\r\n?/g, "\n").split("\n");
    var output = [];
    var toc = [];
    var usedIds = new Set();
    var i = 0;
    while (i < lines.length) {
      var line = lines[i];
      if (!line.trim()) { i++; continue; }

      if (line.trim().indexOf(TICK + TICK + TICK) === 0) {
        var language = line.trim().substring(3).trim();
        var code = [];
        i++;
        while (i < lines.length && lines[i].trim().indexOf(TICK + TICK + TICK) !== 0) code.push(lines[i++]);
        if (i < lines.length) i++;
        output.push('<div class="code-block"><button type="button" class="copy-code">Copy</button><pre><code'
          + (language ? ' class="language-' + escapeHtml(language) + '"' : "")
          + ">" + escapeHtml(code.join("\n")) + "</code></pre></div>");
        continue;
      }

      var heading = /^(#{1,6})\s+(.+?)\s*$/.exec(line);
      if (heading) {
        var level = heading[1].length;
        var plain = heading[2].replace(/[*_~]/g, "").replace(new RegExp(TICK, "g"), "");
        var id = headingId(plain, usedIds);
        if (level <= 3) toc.push({ id: id, level: level, title: plain });
        output.push("<h" + level + ' id="' + escapeHtml(id) + '"><a class="heading-anchor" href="#'
          + escapeHtml(id) + '" aria-label="Link to this section">#</a>'
          + inlineMarkdown(heading[2], doc) + "</h" + level + ">");
        i++;
        continue;
      }

      if (/^\s*(?:---+|\*\*\*+)\s*$/.test(line)) {
        output.push("<hr>");
        i++;
        continue;
      }

      if (i + 1 < lines.length && line.indexOf("|") >= 0 && isTableDivider(lines[i + 1])) {
        var headers = splitTableRow(line);
        i += 2;
        var rows = [];
        while (i < lines.length && lines[i].trim() && lines[i].indexOf("|") >= 0) rows.push(splitTableRow(lines[i++]));
        var table = "<table><thead><tr>" + headers.map(function (cell) {
          return "<th>" + inlineMarkdown(cell, doc) + "</th>";
        }).join("") + "</tr></thead><tbody>";
        rows.forEach(function (row) {
          table += "<tr>" + headers.map(function (_, column) {
            return "<td>" + inlineMarkdown(row[column] || "", doc) + "</td>";
          }).join("") + "</tr>";
        });
        output.push(table + "</tbody></table>");
        continue;
      }

      if (/^>\s?/.test(line)) {
        var quotes = [];
        while (i < lines.length && /^>\s?/.test(lines[i])) quotes.push(lines[i++].replace(/^>\s?/, ""));
        output.push("<blockquote><p>" + inlineMarkdown(quotes.join(" "), doc) + "</p></blockquote>");
        continue;
      }

      var listMatch = /^\s*([-+*]|\d+\.)\s+(.+)$/.exec(line);
      if (listMatch) {
        var ordered = /\d+\./.test(listMatch[1]);
        var tag = ordered ? "ol" : "ul";
        var items = [];
        while (i < lines.length) {
          var match = /^\s*([-+*]|\d+\.)\s+(.+)$/.exec(lines[i]);
          if (!match || /\d+\./.test(match[1]) !== ordered) break;
          items.push("<li>" + inlineMarkdown(match[2], doc) + "</li>");
          i++;
        }
        output.push("<" + tag + ">" + items.join("") + "</" + tag + ">");
        continue;
      }

      var paragraph = [line.trim()];
      i++;
      while (i < lines.length && lines[i].trim() && !isBlockStart(lines, i)) {
        paragraph.push(lines[i].trim());
        i++;
      }
      output.push("<p>" + inlineMarkdown(paragraph.join(" "), doc) + "</p>");
    }
    return { html: output.join("\n"), toc: toc };
  }

  function renderJson(content, doc) {
    var pretty = content;
    try { pretty = JSON.stringify(JSON.parse(content), null, 2); } catch (error) {}
    var id = slugify(doc.title);
    return {
      html: '<h1 id="' + id + '">' + escapeHtml(doc.title) + '</h1><div class="code-block"><button type="button" class="copy-code">Copy</button><pre><code class="language-json">'
        + escapeHtml(pretty) + "</code></pre></div>",
      toc: [{ id: id, level: 1, title: doc.title }]
    };
  }

  function renderToc(items) {
    var toc = byId("article-toc");
    if (!toc) return;
    if (!items.length) {
      toc.innerHTML = '<span class="toc-empty">No headings on this page.</span>';
      return;
    }
    toc.innerHTML = items.map(function (item) {
      return '<a href="#' + escapeHtml(item.id) + '" data-level="' + item.level + '">'
        + escapeHtml(item.title) + "</a>";
    }).join("");
  }

  function activateDocument(slug, anchor, focusArticle) {
    var doc = state.docBySlug.get(slug) || state.docs[0];
    if (!doc) return;
    state.activeSlug = doc.slug;
    var rendered = doc.format === "json" ? renderJson(doc.content, doc) : renderMarkdown(doc.content, doc);
    var index = sortedDocuments().findIndex(function (candidate) { return candidate.slug === doc.slug; });
    var ordered = sortedDocuments();
    var previous = index > 0 ? ordered[index - 1] : null;
    var next = index >= 0 && index < ordered.length - 1 ? ordered[index + 1] : null;
    var article = byId("doc-article");
    if (article) {
      article.innerHTML = '<nav class="breadcrumbs" id="doc-breadcrumbs" aria-label="Breadcrumb">'
        + '<a href="#wiki">Wiki</a><span>/</span><span>' + escapeHtml(doc.category)
        + '</span><span>/</span><span>' + escapeHtml(doc.title) + "</span></nav>"
        + '<div class="doc-article__content">' + rendered.html + "</div>"
        + '<nav class="doc-pagination" aria-label="Adjacent wiki pages">'
        + '<button class="button button--text" id="doc-prev" type="button"' + (previous ? "" : " disabled")
        + '><svg aria-hidden="true" viewBox="0 0 24 24"><path d="m15 18-6-6 6-6"/></svg>'
        + (previous ? escapeHtml(previous.title) : "Previous") + "</button>"
        + '<button class="button button--text" id="doc-next" type="button"' + (next ? "" : " disabled")
        + ">" + (next ? escapeHtml(next.title) : "Next")
        + '<svg aria-hidden="true" viewBox="0 0 24 24"><path d="m9 18 6-6-6-6"/></svg></button></nav>';
      var previousButton = byId("doc-prev");
      var nextButton = byId("doc-next");
      if (previousButton && previous) previousButton.addEventListener("click", function () { navigateToDocument(previous.slug); });
      if (nextButton && next) nextButton.addEventListener("click", function () { navigateToDocument(next.slug); });
      if (focusArticle) article.focus({ preventScroll: true });
    }
    renderNavigation();
    renderToc(rendered.toc);
    document.title = doc.title + " | qBittorrent Material";
    if (anchor) {
      setTimeout(function () {
        var target = document.getElementById(slugify(anchor)) || document.getElementById(anchor);
        if (target) target.scrollIntoView({ behavior: "smooth", block: "start" });
      }, 0);
    }
  }

  function navigateToDocument(slug, anchor) {
    var hash = "#wiki/" + encodeURIComponent(slug) + (anchor ? "/" + encodeURIComponent(anchor) : "");
    if (location.hash === hash) activateDocument(slug, anchor, true);
    else location.hash = hash;
  }

  function handleRoute() {
    var match = /^#wiki\/([^/]+)(?:\/(.+))?$/.exec(location.hash);
    if (match) {
      activateDocument(decodeURIComponent(match[1]), match[2] ? decodeURIComponent(match[2]) : "", false);
      var wiki = byId("wiki");
      if (wiki && location.hash.indexOf("#wiki/") === 0) wiki.scrollIntoView({ block: "start" });
      return;
    }
    if (!state.activeSlug && state.docs.length) {
      var preferred = state.docBySlug.get("wiki-home") || state.docBySlug.get("overview") || state.docs[0];
      activateDocument(preferred.slug, "", false);
    }
  }

  function looksRiskyRegex(pattern) {
    return pattern.length > 320 || /(\([^)]{0,120}[+*][^)]*\))[+*{]/.test(pattern);
  }

  function compileSearch(query, options) {
    if (!query) return null;
    var source = options.regex ? query : escapeRegExp(query);
    if (options.wholeWord) source = "\\b(?:" + source + ")\\b";
    if (looksRiskyRegex(source)) throw new Error("Pattern is too complex for safe in-page search.");
    var flags = options.regex ? String(options.flags || "") : "";
    flags = flags.replace(/[^gimsu]/g, "");
    if (flags.indexOf("g") < 0) flags += "g";
    if (options.caseSensitive) flags = flags.replace(/i/g, "");
    else if (flags.indexOf("i") < 0) flags += "i";
    flags = Array.from(new Set(flags.split(""))).join("");
    return new RegExp(source, flags);
  }

  function cloneRegex(regex, global) {
    var flags = regex.flags.replace(/g/g, "");
    if (global) flags += "g";
    return new RegExp(regex.source, flags);
  }
  function regexTest(regex, value) {
    regex.lastIndex = 0;
    return regex.test(String(value || ""));
  }

  function filterValue(doc, field) {
    if (field === "title") return doc.title;
    if (field === "path") return doc.path;
    if (field === "category") return doc.category;
    if (field === "format") return doc.format;
    if (field === "content") return doc.content;
    return [doc.title, doc.path, doc.category, doc.format, doc.content].join("\n");
  }

  function matchesFilter(doc, rule) {
    var value = filterValue(doc, rule.field || "all");
    var needle = String(rule.value || "");
    if (!needle) return true;
    var source = state.caseSensitive ? value : value.toLowerCase();
    var target = state.caseSensitive ? needle : needle.toLowerCase();
    var result = false;
    if (rule.operator === "equals") result = source === target;
    else if (rule.operator === "starts") result = source.indexOf(target) === 0;
    else if (rule.operator === "ends") result = source.lastIndexOf(target) === source.length - target.length;
    else if (rule.operator === "regex" || rule.operator === "not-regex") {
      try {
        var regex = compileSearch(needle, {
          regex: true,
          wholeWord: false,
          caseSensitive: state.caseSensitive,
          flags: state.regexFlags
        });
        result = regexTest(regex, value);
        if (rule.operator === "not-regex") result = !result;
      } catch (error) { result = false; }
    } else result = source.indexOf(target) >= 0;
    return rule.negate ? !result : result;
  }

  function passesFilters(doc) {
    if (!state.filters.length) return true;
    var tests = state.filters.map(function (rule) { return matchesFilter(doc, rule); });
    return state.filterMode === "any" ? tests.some(Boolean) : tests.every(Boolean);
  }

  function firstMatch(content, regex) {
    if (!regex) return { index: 0, text: "" };
    var local = cloneRegex(regex, false);
    var match = local.exec(content);
    return match ? { index: match.index, text: match[0] } : null;
  }

  function makeExcerpt(doc, regex) {
    var clean = doc.content
      .replace(/<[^>]+>/g, " ")
      .replace(/!\[([^\]]*)\]\([^)]+\)/g, "$1")
      .replace(/\[([^\]]+)\]\([^)]+\)/g, "$1")
      .replace(/^#{1,6}\s+/gm, "")
      .replace(/[`*_>|]/g, " ")
      .replace(/\s+/g, " ")
      .trim();
    var match = firstMatch(clean, regex);
    var index = match ? match.index : 0;
    var start = Math.max(0, index - 90);
    var end = Math.min(clean.length, index + (match ? match.text.length : 0) + 170);
    var before = clean.slice(start, match ? index : end);
    var marked = match ? clean.slice(index, index + match.text.length) : "";
    var after = match ? clean.slice(index + match.text.length, end) : "";
    return (start ? "…" : "") + escapeHtml(before)
      + (match ? '<mark class="search-highlight">' + escapeHtml(marked || "match") + "</mark>" : "")
      + escapeHtml(after) + (end < clean.length ? "…" : "");
  }

  function performSearch() {
    var resultsPanel = byId("search-results");
    var article = byId("doc-article");
    var status = byId("search-status");
    var list = byId("search-result-list");
    var query = state.query.trim();
    var matcher = null;
    var error = "";
    try {
      matcher = compileSearch(query, {
        regex: state.regex,
        wholeWord: state.wholeWord,
        caseSensitive: state.caseSensitive,
        flags: state.regexFlags
      });
    } catch (problem) { error = problem.message; }

    if (error) {
      if (resultsPanel) resultsPanel.hidden = false;
      if (article) article.hidden = true;
      if (status) status.textContent = error;
      if (list) list.innerHTML = '<div class="empty-state"><strong>Invalid regular expression</strong><p>'
        + escapeHtml(error) + "</p></div>";
      return;
    }

    if (!query && !state.filters.length) {
      if (resultsPanel) resultsPanel.hidden = true;
      if (article) article.hidden = false;
      if (status) status.textContent = "Type to search all documentation";
      return;
    }

    var matches = state.docs.filter(function (doc) {
      if (!passesFilters(doc)) return false;
      return !matcher || regexTest(matcher, [doc.title, doc.path, doc.category, doc.content].join("\n"));
    }).slice(0, 150);
    if (resultsPanel) resultsPanel.hidden = false;
    if (article) article.hidden = true;
    if (status) status.textContent = matches.length + " matching document" + (matches.length === 1 ? "" : "s");
    if (list) {
      list.innerHTML = matches.length ? matches.map(function (doc) {
        return '<a class="search-result" href="#wiki/' + encodeURIComponent(doc.slug)
          + '" data-doc-slug="' + escapeHtml(doc.slug) + '"><div class="search-result__meta"><span>'
          + escapeHtml(doc.category) + "</span><span>" + escapeHtml(doc.format) + "</span></div><h3>"
          + escapeHtml(doc.title) + "</h3><p>" + makeExcerpt(doc, matcher) + "</p></a>";
      }).join("") : '<div class="empty-state"><strong>No matching documents</strong><p>Try a broader query, remove a filter, or test the pattern in the regex builder.</p></div>';
    }
    persistSearch();
  }

  var debouncedSearch = debounce(performSearch, 110);
  function persistSearch() {
    saveStored(STORAGE.search, {
      query: state.query,
      regex: state.regex,
      caseSensitive: state.caseSensitive,
      wholeWord: state.wholeWord,
      regexFlags: state.regexFlags,
      filters: state.filters,
      filterMode: state.filterMode
    });
  }

  function renderFilterChips() {
    var target = byId("active-filter-chips");
    if (!target) return;
    target.innerHTML = state.filters.map(function (rule, index) {
      return '<button type="button" data-remove-filter="' + index + '" title="Remove filter">'
        + escapeHtml((rule.negate ? "not " : "") + (rule.field || "all") + " "
        + (rule.operator || "contains") + " " + (rule.value || "")) + " ×</button>";
    }).join("");
  }

  function filterRuleMarkup(rule, index) {
    var fieldOptions = FILTER_FIELDS.map(function (option) {
      return '<option value="' + option[0] + '"' + (rule.field === option[0] ? " selected" : "") + ">"
        + option[1] + "</option>";
    }).join("");
    var operatorOptions = FILTER_OPERATORS.map(function (option) {
      return '<option value="' + option[0] + '"' + (rule.operator === option[0] ? " selected" : "") + ">"
        + option[1] + "</option>";
    }).join("");
    return '<div class="filter-rule" data-filter-index="' + index + '"><label><span>Field</span><select data-filter-field>'
      + fieldOptions + '</select></label><label><span>Condition</span><select data-filter-operator>'
      + operatorOptions + '</select></label><label class="filter-rule__value"><span>Value</span><input data-filter-value type="text" value="'
      + escapeHtml(rule.value || "") + '" placeholder="e.g. installer"></label><label class="toggle-chip"><input data-filter-negate type="checkbox"'
      + (rule.negate ? " checked" : "") + "><span>!</span> Negate</label>"
      + '<button class="icon-button remove-filter-rule" type="button" data-remove-rule="' + index
      + '" aria-label="Remove rule">×</button></div>';
  }

  function renderFilterRules() {
    var container = byId("filter-rules");
    if (!container) return;
    if (!filterDraft.length) filterDraft.push({ field: "all", operator: "contains", value: "", negate: false });
    container.innerHTML = filterDraft.map(filterRuleMarkup).join("");
    var match = document.querySelector('input[name="filter-match"][value="' + state.filterMode + '"]');
    if (match) match.checked = true;
  }

  function readFilterRules() {
    return all("[data-filter-index]", byId("filter-rules")).map(function (row) {
      return {
        field: row.querySelector("[data-filter-field]").value,
        operator: row.querySelector("[data-filter-operator]").value,
        value: row.querySelector("[data-filter-value]").value.trim(),
        negate: row.querySelector("[data-filter-negate]").checked
      };
    }).filter(function (rule) { return rule.value; });
  }

  function updateRegexPreview() {
    var patternInput = byId("regex-pattern");
    var flagsInput = byId("regex-flags");
    var sampleInput = byId("regex-sample");
    var preview = byId("regex-preview");
    if (!patternInput || !preview) return;
    var pattern = patternInput.value;
    var sample = sampleInput ? sampleInput.value : "";
    if (!pattern) {
      preview.classList.remove("is-error");
      preview.innerHTML = "<span>Pattern preview</span><code>No pattern entered</code>";
      return;
    }
    try {
      var regex = compileSearch(pattern, {
        regex: true,
        wholeWord: false,
        caseSensitive: false,
        flags: flagsInput ? flagsInput.value : "gi"
      });
      var matches = [];
      var match;
      var guard = 0;
      regex.lastIndex = 0;
      while ((match = regex.exec(sample)) && guard++ < 100) {
        matches.push({ index: match.index, text: match[0], groups: match.slice(1) });
        if (match[0] === "") regex.lastIndex++;
      }
      var cursor = 0;
      var highlighted = "";
      matches.forEach(function (item) {
        highlighted += escapeHtml(sample.slice(cursor, item.index))
          + "<mark>" + escapeHtml(item.text || "∅") + "</mark>";
        cursor = item.index + item.text.length;
      });
      highlighted += escapeHtml(sample.slice(cursor));
      var groups = [];
      matches.forEach(function (item, matchIndex) {
        item.groups.forEach(function (group, groupIndex) {
          groups.push("match " + (matchIndex + 1) + " · group " + (groupIndex + 1) + ": " + (group == null ? "undefined" : group));
        });
      });
      preview.classList.remove("is-error");
      preview.innerHTML = "<span>" + matches.length + " match" + (matches.length === 1 ? "" : "es")
        + (groups.length ? " · " + groups.length + " capture groups" : "") + "</span><code>"
        + (highlighted || "No matches in sample text") + "</code>"
        + (groups.length ? "<small>" + escapeHtml(groups.slice(0, 12).join(" | ")) + "</small>" : "");
    } catch (error) {
      preview.classList.add("is-error");
      preview.innerHTML = "<span>Pattern error</span><code>" + escapeHtml(error.message) + "</code>";
    }
  }

  function insertRegexToken(token) {
    var input = byId("regex-pattern");
    if (!input) return;
    var start = input.selectionStart == null ? input.value.length : input.selectionStart;
    var end = input.selectionEnd == null ? input.value.length : input.selectionEnd;
    input.value = input.value.slice(0, start) + token + input.value.slice(end);
    var caret = token === "(?:)" ? start + 3 : start + token.length;
    input.focus();
    input.setSelectionRange(caret, caret);
    updateRegexPreview();
  }

  function downloadJson(filename, value) {
    var blob = new Blob([JSON.stringify(value, null, 2) + "\n"], { type: "application/json" });
    var url = URL.createObjectURL(blob);
    var anchor = document.createElement("a");
    anchor.href = url;
    anchor.download = filename;
    document.body.appendChild(anchor);
    anchor.click();
    anchor.remove();
    setTimeout(function () { URL.revokeObjectURL(url); }, 1000);
  }

  function exportedWiki() {
    return {
      type: "qbt-material-wiki",
      schemaVersion: 1,
      repository: "codingmachineedge/qbittorrent-material",
      exportedAt: new Date().toISOString(),
      documents: state.docs.map(function (doc) {
        return {
          slug: doc.slug,
          title: doc.title,
          path: doc.path,
          category: doc.category,
          format: doc.format,
          content: doc.content,
          imported: doc.imported
        };
      })
    };
  }

  function exportedSearch() {
    return {
      type: "qbt-material-search-profile",
      schemaVersion: 1,
      exportedAt: new Date().toISOString(),
      query: state.query,
      regex: state.regex,
      caseSensitive: state.caseSensitive,
      wholeWord: state.wholeWord,
      regexFlags: state.regexFlags,
      filterMode: state.filterMode,
      filters: state.filters
    };
  }

  function applySearchProfile(profile) {
    state.query = String(profile.query || "");
    state.regex = Boolean(profile.regex);
    state.caseSensitive = Boolean(profile.caseSensitive);
    state.wholeWord = Boolean(profile.wholeWord);
    state.regexFlags = String(profile.regexFlags || "gi").replace(/[^gimsu]/g, "") || "gi";
    state.filterMode = profile.filterMode === "any" ? "any" : "all";
    state.filters = Array.isArray(profile.filters) ? profile.filters.slice(0, 40) : [];
    syncSearchControls();
    renderFilterChips();
    performSearch();
  }

  function uniqueImportedSlug(title, path) {
    var base = "local-" + slugify(title || path);
    var slug = base;
    var index = 2;
    var used = new Set(state.imported.map(function (doc) { return doc.slug; }));
    while (used.has(slug)) slug = base + "-" + index++;
    return slug;
  }

  function addImportedDocument(doc) {
    var title = String(doc.title || "Imported document").slice(0, 180);
    var path = String(doc.path || ("local/" + slugify(title) + ".md")).replace(/\\/g, "/");
    if (state.docs.some(function (existing) { return !existing.imported && existing.path.toLowerCase() === path.toLowerCase(); })) return false;
    state.imported.push({
      slug: uniqueImportedSlug(title, path),
      title: title,
      path: path,
      category: String(doc.category || "Imported"),
      format: doc.format === "json" ? "json" : "markdown",
      content: String(doc.content == null ? "" : doc.content),
      importedAt: new Date().toISOString()
    });
    return true;
  }

  async function importFiles(fileList) {
    var files = Array.prototype.slice.call(fileList || []);
    var added = 0;
    for (var i = 0; i < files.length; i++) {
      var file = files[i];
      var text = await file.text();
      if (/\.json$/i.test(file.name)) {
        var parsed = safeJsonParse(text, null);
        if (parsed && parsed.type === "qbt-material-search-profile") {
          applySearchProfile(parsed);
          showSnackbar("Imported search profile " + file.name);
          continue;
        }
        if (parsed && Array.isArray(parsed.documents)) {
          parsed.documents.slice(0, 500).forEach(function (doc) {
            if (addImportedDocument(doc)) added++;
          });
          continue;
        }
        if (parsed) {
          if (addImportedDocument({
            title: file.name.replace(/\.json$/i, ""),
            path: "local/" + file.name,
            format: "json",
            content: JSON.stringify(parsed, null, 2)
          })) added++;
          continue;
        }
        showSnackbar("Could not parse " + file.name + " as JSON.");
        continue;
      }
      if (addImportedDocument({
        title: file.name.replace(/\.(?:md|markdown|txt)$/i, "").replace(/[-_]+/g, " "),
        path: "local/" + file.name,
        format: "markdown",
        content: text
      })) added++;
    }
    if (added) {
      saveStored(STORAGE.imported, state.imported);
      rebuildCorpus();
      renderNavigation();
      renderImportList();
      performSearch();
      showSnackbar("Imported " + added + " local page" + (added === 1 ? "" : "s") + ".");
    }
  }

  function renderImportList() {
    var list = byId("import-file-list");
    if (!list) return;
    list.innerHTML = state.imported.length ? state.imported.map(function (doc, index) {
      return '<span><strong>' + escapeHtml(doc.title) + '</strong><button type="button" data-remove-import="'
        + index + '" aria-label="Remove ' + escapeHtml(doc.title) + '">×</button></span>';
    }).join("") : "<small>No local pages imported.</small>";
  }

  function syncSearchControls() {
    var search = byId("global-search");
    var regex = byId("regex-toggle");
    var matchCase = byId("case-toggle");
    var wholeWord = byId("whole-word-toggle");
    if (search) search.value = state.query;
    if (regex) regex.checked = state.regex;
    if (matchCase) matchCase.checked = state.caseSensitive;
    if (wholeWord) wholeWord.checked = state.wholeWord;
  }

  function initializeTheme() {
    var stored = null;
    try { stored = localStorage.getItem(STORAGE.theme); } catch (error) {}
    var dark = stored ? stored === "dark" : window.matchMedia && window.matchMedia("(prefers-color-scheme: dark)").matches;
    document.documentElement.dataset.theme = dark ? "dark" : "light";
    var meta = document.querySelector('meta[name="theme-color"]');
    if (meta) meta.content = dark ? "#1d1b20" : "#6750a4";
  }

  function toggleTheme() {
    var dark = document.documentElement.dataset.theme !== "dark";
    document.documentElement.dataset.theme = dark ? "dark" : "light";
    try { localStorage.setItem(STORAGE.theme, dark ? "dark" : "light"); } catch (error) {}
    var meta = document.querySelector('meta[name="theme-color"]');
    if (meta) meta.content = dark ? "#1d1b20" : "#6750a4";
    showSnackbar((dark ? "Dark" : "Light") + " theme enabled.");
  }

  function bindEvents() {
    document.addEventListener("click", function (event) {
      var docButton = event.target.closest("[data-doc-slug]");
      if (docButton) {
        event.preventDefault();
        navigateToDocument(docButton.getAttribute("data-doc-slug"), docButton.getAttribute("data-doc-anchor") || "");
        var panel = byId("search-results");
        var article = byId("doc-article");
        if (panel) panel.hidden = true;
        if (article) article.hidden = false;
        return;
      }
      var legacyDoc = event.target.closest("[data-doc]");
      if (legacyDoc) {
        var target = state.docByPath.get(String(legacyDoc.getAttribute("data-doc")).toLowerCase());
        if (target) navigateToDocument(target.slug);
      }
      var removeFilter = event.target.closest("[data-remove-filter]");
      if (removeFilter) {
        state.filters.splice(Number(removeFilter.getAttribute("data-remove-filter")), 1);
        renderFilterChips();
        performSearch();
      }
      var removeRule = event.target.closest("[data-remove-rule]");
      if (removeRule) {
        filterDraft.splice(Number(removeRule.getAttribute("data-remove-rule")), 1);
        renderFilterRules();
      }
      var token = event.target.closest("[data-regex-token]");
      if (token) insertRegexToken(token.getAttribute("data-regex-token"));
      var removeImport = event.target.closest("[data-remove-import]");
      if (removeImport) {
        state.imported.splice(Number(removeImport.getAttribute("data-remove-import")), 1);
        saveStored(STORAGE.imported, state.imported);
        rebuildCorpus();
        renderNavigation();
        renderImportList();
        performSearch();
      }
      var copy = event.target.closest(".copy-code");
      if (copy) {
        var code = copy.parentElement.querySelector("code");
        if (code && navigator.clipboard) navigator.clipboard.writeText(code.textContent).then(function () {
          copy.textContent = "Copied";
          setTimeout(function () { copy.textContent = "Copy"; }, 1400);
        });
      }
      var gallery = event.target.closest("[data-lightbox]");
      if (gallery) {
        var image = byId("lightbox-image");
        var caption = byId("lightbox-caption");
        if (image) image.src = gallery.getAttribute("data-lightbox");
        if (image) image.alt = gallery.getAttribute("data-caption") || "Application screenshot";
        if (caption) caption.textContent = gallery.getAttribute("data-caption") || "Application screenshot";
        showDialog(byId("lightbox-dialog"));
      }
    });

    var search = byId("global-search");
    if (search) search.addEventListener("input", function () {
      state.query = search.value;
      debouncedSearch();
    });
    var searchForm = byId("global-search-form");
    if (searchForm) searchForm.addEventListener("submit", function (event) {
      event.preventDefault();
      state.query = search ? search.value : state.query;
      performSearch();
    });
    ["regex-toggle", "case-toggle", "whole-word-toggle"].forEach(function (id) {
      var control = byId(id);
      if (!control) return;
      control.addEventListener("change", function () {
        state.regex = Boolean(byId("regex-toggle") && byId("regex-toggle").checked);
        state.caseSensitive = Boolean(byId("case-toggle") && byId("case-toggle").checked);
        state.wholeWord = Boolean(byId("whole-word-toggle") && byId("whole-word-toggle").checked);
        performSearch();
      });
    });

    var filterDialog = byId("filter-dialog");
    var openFilters = byId("open-filter-builder");
    if (openFilters) openFilters.addEventListener("click", function () {
      filterDraft = state.filters.map(function (rule) { return Object.assign({}, rule); });
      renderFilterRules();
      showDialog(filterDialog);
    });
    var addRule = byId("add-filter-rule");
    if (addRule) addRule.addEventListener("click", function () {
      filterDraft.push({ field: "all", operator: "contains", value: "", negate: false });
      renderFilterRules();
    });
    var clearFilters = byId("clear-filters");
    if (clearFilters) clearFilters.addEventListener("click", function () {
      filterDraft = [];
      renderFilterRules();
    });
    var applyFilters = byId("apply-filters");
    if (applyFilters) applyFilters.addEventListener("click", function (event) {
      event.preventDefault();
      state.filters = readFilterRules();
      var selectedMode = document.querySelector('input[name="filter-match"]:checked');
      state.filterMode = selectedMode && selectedMode.value === "any" ? "any" : "all";
      renderFilterChips();
      performSearch();
      closeDialog(filterDialog);
    });

    var regexDialog = byId("regex-dialog");
    var openRegex = byId("open-regex-builder");
    if (openRegex) openRegex.addEventListener("click", function () {
      var pattern = byId("regex-pattern");
      var flags = byId("regex-flags");
      if (pattern) pattern.value = state.query;
      if (flags) flags.value = state.regexFlags;
      updateRegexPreview();
      showDialog(regexDialog);
    });
    ["regex-pattern", "regex-flags", "regex-sample"].forEach(function (id) {
      var input = byId(id);
      if (input) input.addEventListener("input", updateRegexPreview);
    });
    var clearRegex = byId("regex-clear");
    if (clearRegex) clearRegex.addEventListener("click", function () {
      if (byId("regex-pattern")) byId("regex-pattern").value = "";
      if (byId("regex-flags")) byId("regex-flags").value = "gi";
      updateRegexPreview();
    });
    var applyRegex = byId("regex-apply");
    if (applyRegex) applyRegex.addEventListener("click", function (event) {
      event.preventDefault();
      var pattern = byId("regex-pattern") ? byId("regex-pattern").value : "";
      var flags = byId("regex-flags") ? byId("regex-flags").value : "gi";
      try {
        compileSearch(pattern, { regex: true, wholeWord: false, caseSensitive: false, flags: flags });
        state.query = pattern;
        state.regex = true;
        state.regexFlags = flags;
        syncSearchControls();
        performSearch();
        closeDialog(regexDialog);
      } catch (error) { showSnackbar(error.message); }
    });
    var regexExport = byId("regex-export");
    if (regexExport) regexExport.addEventListener("click", function () {
      downloadJson("qbt-material-regex.json", {
        type: "qbt-material-regex",
        schemaVersion: 1,
        pattern: byId("regex-pattern") ? byId("regex-pattern").value : "",
        flags: byId("regex-flags") ? byId("regex-flags").value : "gi",
        sample: byId("regex-sample") ? byId("regex-sample").value : ""
      });
    });
    var regexImport = byId("regex-import");
    if (regexImport) regexImport.addEventListener("click", function () {
      var input = document.createElement("input");
      input.type = "file";
      input.accept = ".json,application/json";
      input.addEventListener("change", async function () {
        if (!input.files || !input.files[0]) return;
        var value = safeJsonParse(await input.files[0].text(), null);
        if (!value || typeof value.pattern !== "string") return showSnackbar("That file is not a regex profile.");
        if (byId("regex-pattern")) byId("regex-pattern").value = value.pattern;
        if (byId("regex-flags")) byId("regex-flags").value = value.flags || "gi";
        if (byId("regex-sample") && typeof value.sample === "string") byId("regex-sample").value = value.sample;
        updateRegexPreview();
      });
      input.click();
    });

    var transferDialog = byId("import-export-dialog");
    var openTransfer = byId("open-import-export");
    if (openTransfer) openTransfer.addEventListener("click", function () {
      renderImportList();
      showDialog(transferDialog);
    });
    var importInput = byId("import-files");
    if (importInput) importInput.addEventListener("change", function () {
      importFiles(importInput.files).finally(function () { importInput.value = ""; });
    });
    var clearImports = byId("clear-imports");
    if (clearImports) clearImports.addEventListener("click", function () {
      state.imported = [];
      saveStored(STORAGE.imported, state.imported);
      rebuildCorpus();
      renderNavigation();
      renderImportList();
      performSearch();
      showSnackbar("Cleared local imported pages.");
    });
    var exportWiki = byId("export-wiki");
    if (exportWiki) exportWiki.addEventListener("click", function () {
      downloadJson("qbt-material-wiki.json", exportedWiki());
    });
    var exportSearch = byId("export-search");
    if (exportSearch) exportSearch.addEventListener("click", function () {
      downloadJson("qbt-material-search.json", exportedSearch());
    });

    var theme = byId("theme-toggle");
    if (theme) theme.addEventListener("click", toggleTheme);
    var mobile = byId("mobile-nav-toggle");
    var primary = byId("primary-navigation");
    if (mobile && primary) mobile.addEventListener("click", function () {
      var open = mobile.getAttribute("aria-expanded") === "true";
      mobile.setAttribute("aria-expanded", String(!open));
      primary.classList.toggle("is-open", !open);
    });
    var snackbarAction = byId("snackbar-action");
    if (snackbarAction) snackbarAction.addEventListener("click", function () {
      var bar = byId("snackbar");
      if (bar) bar.hidden = true;
    });
    document.addEventListener("keydown", function (event) {
      var tag = document.activeElement && document.activeElement.tagName;
      var typing = tag === "INPUT" || tag === "TEXTAREA" || tag === "SELECT";
      if ((event.key === "/" && !typing) || ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "k")) {
        event.preventDefault();
        if (search) {
          byId("wiki").scrollIntoView({ behavior: "smooth", block: "start" });
          search.focus();
          search.select();
        }
      } else if (event.key === "Escape" && search && document.activeElement === search && search.value) {
        search.value = "";
        state.query = "";
        performSearch();
      }
    });
    window.addEventListener("hashchange", handleRoute);
  }

  function registerServiceWorker() {
    if ("serviceWorker" in navigator && location.protocol.indexOf("http") === 0) {
      navigator.serviceWorker.register("./sw.js").catch(function () {});
    }
  }

  function initialize() {
    initializeTheme();
    rebuildCorpus();
    syncSearchControls();
    renderNavigation();
    renderFilterChips();
    renderImportList();
    bindEvents();
    handleRoute();
    if (state.query || state.filters.length) performSearch();
    registerServiceWorker();
  }

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", initialize);
  else initialize();
})();
