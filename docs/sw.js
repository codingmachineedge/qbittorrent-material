const CACHE_VERSION = "qbt-material-docs-v1";
const APP_SHELL = [
  "./",
  "./index.html",
  "./content.generated.js",
  "./assets/site.css",
  "./assets/site.js",
  "./assets/logo-mark.svg",
  "./manifest.webmanifest",
  "./images/app/01-main-window.png",
  "./images/app/02-toolbar-and-filter.png",
  "./images/app/03-filter-sidebar.png",
  "./images/app/04-transfer-list.png",
  "./images/app/05-properties-tabs.png",
  "./images/app/06-statusbar.png",
  "./images/app/07-navigation-and-toolbar.png",
  "./images/app/08-main-workspace.png",
  "./images/site/01-landing-desktop.png",
  "./images/site/02-wiki-search.png",
  "./images/site/03-regex-builder.png",
  "./images/site/04-mobile-landing.png"
];

self.addEventListener("install", event => {
  event.waitUntil(
    caches.open(CACHE_VERSION)
      .then(cache => cache.addAll(APP_SHELL))
      .then(() => self.skipWaiting())
  );
});

self.addEventListener("activate", event => {
  event.waitUntil(
    caches.keys()
      .then(keys => Promise.all(
        keys.filter(key => key !== CACHE_VERSION).map(key => caches.delete(key))
      ))
      .then(() => self.clients.claim())
  );
});

self.addEventListener("fetch", event => {
  if (event.request.method !== "GET") return;

  const url = new URL(event.request.url);
  if (url.origin !== self.location.origin) return;

  const wantsFresh = event.request.mode === "navigate"
    || /(?:index\.html|content\.generated\.js)$/.test(url.pathname);

  if (wantsFresh) {
    event.respondWith(
      fetch(event.request)
        .then(response => {
          const copy = response.clone();
          caches.open(CACHE_VERSION).then(cache => cache.put(event.request, copy));
          return response;
        })
        .catch(() => caches.match(event.request).then(hit => hit || caches.match("./index.html")))
    );
    return;
  }

  event.respondWith(
    caches.match(event.request).then(hit => {
      const refresh = fetch(event.request)
        .then(response => {
          if (response.ok) {
            const copy = response.clone();
            caches.open(CACHE_VERSION).then(cache => cache.put(event.request, copy));
          }
          return response;
        })
        .catch(() => hit);
      return hit || refresh;
    })
  );
});
