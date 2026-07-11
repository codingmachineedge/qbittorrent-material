"use strict";

var MAX_PATTERN_LENGTH = 320;
var MAX_RESULTS = 150;
var MAX_PREVIEW_MATCHES = 100;
var MAX_SAMPLE_LENGTH = 200000;

function compilePattern(pattern, options) {
  var source = String(pattern || "");
  if (!source) return null;
  if (source.length > MAX_PATTERN_LENGTH) throw new Error("Pattern exceeds 320 characters.");
  if (!options.regex) source = source.replace(/[.*+?^$()|[\]\\{}]/g, "\\$&");
  if (options.wholeWord) source = "\\b(?:" + source + ")\\b";
  var flags = options.regex ? String(options.flags || "") : "";
  flags = flags.replace(/[^gimsu]/g, "");
  if (flags.indexOf("g") < 0) flags += "g";
  if (options.caseSensitive) flags = flags.replace(/i/g, "");
  else if (flags.indexOf("i") < 0) flags += "i";
  flags = Array.from(new Set(flags.split(""))).join("");
  return new RegExp(source, flags);
}

function testPattern(regex, value) {
  if (!regex) return true;
  regex.lastIndex = 0;
  return regex.test(String(value || ""));
}

function documentField(doc, field) {
  if (field === "title") return doc.title;
  if (field === "path") return doc.path;
  if (field === "category") return doc.category;
  if (field === "format") return doc.format;
  if (field === "content") return doc.content;
  return [doc.title, doc.path, doc.category, doc.format, doc.content].join("\n");
}

function compileFilters(filters, options) {
  return (filters || []).map(function (rule) {
    var copy = {
      field: rule.field || "all",
      operator: rule.operator || "contains",
      value: String(rule.value || ""),
      negate: Boolean(rule.negate),
      regex: null
    };
    if (copy.operator === "regex" || copy.operator === "not-regex") {
      copy.regex = compilePattern(copy.value, {
        regex: true,
        wholeWord: false,
        caseSensitive: options.caseSensitive,
        flags: options.flags
      });
    }
    return copy;
  });
}

function matchesFilter(doc, rule, caseSensitive) {
  var value = documentField(doc, rule.field);
  var source = caseSensitive ? value : value.toLowerCase();
  var target = caseSensitive ? rule.value : rule.value.toLowerCase();
  var result;
  if (!rule.value) result = true;
  else if (rule.operator === "equals") result = source === target;
  else if (rule.operator === "starts") result = source.indexOf(target) === 0;
  else if (rule.operator === "ends") result = source.lastIndexOf(target) === source.length - target.length;
  else if (rule.operator === "regex" || rule.operator === "not-regex") {
    result = testPattern(rule.regex, value);
    if (rule.operator === "not-regex") result = !result;
  } else result = source.indexOf(target) >= 0;
  return rule.negate ? !result : result;
}

function passesFilters(doc, filters, mode, caseSensitive) {
  if (!filters.length) return true;
  var results = filters.map(function (rule) {
    return matchesFilter(doc, rule, caseSensitive);
  });
  return mode === "any" ? results.some(Boolean) : results.every(Boolean);
}

function contentMatch(regex, content) {
  if (!regex) return { index: 0, text: "" };
  regex.lastIndex = 0;
  var match = regex.exec(String(content || ""));
  return match ? { index: match.index, text: match[0] } : { index: 0, text: "" };
}

function runSearch(message) {
  var options = message.options || {};
  var matcher = compilePattern(message.query, options);
  var filters = compileFilters(message.filters, options);
  var matches = [];
  var docs = Array.isArray(message.documents) ? message.documents : [];
  for (var index = 0; index < docs.length && matches.length < MAX_RESULTS; index++) {
    var doc = docs[index];
    if (!passesFilters(doc, filters, message.filterMode, Boolean(options.caseSensitive))) continue;
    var haystack = [doc.title, doc.path, doc.category, doc.format, doc.content].join("\n");
    if (!testPattern(matcher, haystack)) continue;
    var excerpt = contentMatch(matcher, doc.content);
    matches.push({
      slug: doc.slug,
      contentIndex: excerpt.index,
      matchText: excerpt.text
    });
  }
  return { type: "search", matches: matches };
}

function runPreview(message) {
  var sample = String(message.sample || "").slice(0, MAX_SAMPLE_LENGTH);
  var regex = compilePattern(message.pattern, {
    regex: true,
    wholeWord: false,
    caseSensitive: Boolean(message.caseSensitive),
    flags: message.flags
  });
  if (!regex) return { type: "preview", matches: [], sampleLength: sample.length };
  var matches = [];
  var match;
  regex.lastIndex = 0;
  while ((match = regex.exec(sample)) && matches.length < MAX_PREVIEW_MATCHES) {
    matches.push({
      index: match.index,
      text: match[0],
      groups: match.slice(1)
    });
    if (match[0] === "") regex.lastIndex++;
  }
  return {
    type: "preview",
    matches: matches,
    sampleLength: sample.length,
    truncated: String(message.sample || "").length > sample.length
  };
}

self.addEventListener("message", function (event) {
  var message = event.data || {};
  try {
    var result = message.type === "preview" ? runPreview(message) : runSearch(message);
    result.requestId = message.requestId;
    self.postMessage(result);
  } catch (error) {
    self.postMessage({
      type: message.type || "search",
      requestId: message.requestId,
      error: error && error.message ? error.message : String(error)
    });
  }
});
