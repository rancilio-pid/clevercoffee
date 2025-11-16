// load-libs.js
window.__libsCallback = null;
window.onLibsReady = function(cb) {
  window.__libsCallback = cb;
};

let LOAD_MODE = "auto"; // other options are local,cdn
const allowedModes = ["local", "cdn", "auto"];
const urlParams = new URLSearchParams(window.location.search);
const minHeap = 8000000; // 8000000 doesn't cover the size of all files but helps delay briefly for some heap to recover

 // lightweight method to change mode at runtime. eg silvia.local/?mode=local
if (urlParams.has("mode")) {
  const m = urlParams.get("mode").toLowerCase();
  
  if (allowedModes.includes(m)) {
    LOAD_MODE = m;
  }
}

if (LOAD_MODE !== "auto") {
  console.log(`Asset Loader forced to ${LOAD_MODE} — skipping CDN probe`);
  window.__USE_CDN = (LOAD_MODE === "cdn");
}

function runSingleCDNProbe(url, timeoutMs = 500) {
  if (LOAD_MODE !== "auto") {
    return Promise.resolve();
  }

  return Promise.race([
    fetch(url, { method: "HEAD", cache: "no-cache" }),
    new Promise((_, reject) => setTimeout(() => reject("timeout"), timeoutMs))
  ])
  .then(() => {
    console.log("CDN reachable — using CDN for all assets");
    window.__USE_CDN = true;
  })
  .catch(() => {
    console.log("CDN not reachable — using LOCAL for all assets");
    window.__USE_CDN = false;
  });
}

function waitForHeap(minFreeBytes, checkInterval = 100, maxWaitMs = 2000) {
  return new Promise((resolve, reject) => {
    let resolved = false;

    function check() {
      const mem = performance.memory;
      if (!mem) {
        return resolve();
      }

      const free = mem.jsHeapSizeLimit - mem.usedJSHeapSize;

      if (free >= minFreeBytes) {
        resolved = true;
        resolve(true);
      }
      else {
        setTimeout(check, checkInterval);
      }
    }

    check();
    
    // Master timeout
    setTimeout(() => {
      if (!resolved) {
        resolved = true;
        resolve(false);
      }
    }, maxWaitMs);
  });
}

function loadWhenFree(minFreeBytes, ...args) {
  return waitForHeap(minFreeBytes).then(() => loadAuto(...args));
}

function loadAuto(localUrl, cdnUrl, type="js", globalAssign=null) {

  if (LOAD_MODE === "local") {
    return loadLib(localUrl, cdnUrl, type, globalAssign, "local-only");
  }

  if (LOAD_MODE === "cdn") {
    return loadLib(localUrl, cdnUrl, type, globalAssign, "cdn-only");
  }

  if (window.__USE_CDN && cdnUrl) {
    return loadLib(localUrl, cdnUrl, type, globalAssign, "cdn-only");
  }

  return loadLib(localUrl, cdnUrl, type, globalAssign, "local-only");
}

function loadLib(localUrl, cdnUrl, type="js", globalAssign=null, mode="auto") {
  return new Promise((resolve, reject) => {
    function inject(url, tag) {
      let el;
      if (type === "js") {
        el = document.createElement("script");
        el.src = url;
        el.defer = true;
      } 
      else {
        el = document.createElement("link");
        el.rel = "stylesheet";
        el.href = url;
      }

      el.onload = () => {
        if (globalAssign && type === "js") {
          globalAssign();
        }

        resolve(tag);
      };

      el.onerror = () => reject(new Error(`Failed: ${url}`));
      document.head.appendChild(el);
    }

    if (mode === "cdn-only") {
      return inject(cdnUrl, "cdn");
    }

    if (mode === "local-only") {
      return inject(localUrl, "local");
    }

    return inject(localUrl, "local");
  });
}

// Test if CDN is available
runSingleCDNProbe("https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/css/bootstrap.min.css")
  .then(() => {

// Load CSS in sequence
  return loadWhenFree(minHeap, "/css/fontawesome-6.2.1.min.css", "https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.2.1/css/all.min.css", "css");
  })
.then(src => {
  console.log(`FontAwesome CSS loaded from ${src}`);
  return loadWhenFree(minHeap, "/css/bootstrap-5.2.3.min.css", "https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/css/bootstrap.min.css", "css");
})
.then(src => {
  console.log(`Bootstrap CSS loaded from ${src}`);
  return loadWhenFree(minHeap, "/css/uPlot.min.css", "https://cdn.jsdelivr.net/npm/uplot@1.6.24/dist/uPlot.min.css", "css");
})
.then(src => {
  console.log(`uPlot CSS loaded from ${src}`);
  
// Load JS in sequence
  return loadWhenFree(minHeap, "/js/vue.3.2.47.min.js", "https://cdn.jsdelivr.net/npm/vue@3.2.47/dist/vue.global.prod.min.js", "js", () => { window.Vue = Vue; });
})
.then(src => {
  console.log(`Vue loaded from ${src}`);
  return loadWhenFree(minHeap, "/js/vue-number-input.min.js", "https://unpkg.com/@chenfengyuan/vue-number-input@2.0.1/dist/vue-number-input.min.js", "js", () => { window.VueNumberInput = VueNumberInput; });
})
.then(src => {
  console.log(`VueNumberInput loaded from ${src}`);
  return loadWhenFree(minHeap, "/js/uPlot.1.6.28.min.js", "https://cdn.jsdelivr.net/npm/uplot@1.6.28/dist/uPlot.iife.min.js", "js", () => { window.uPlot = uPlot; });
})
.then(src => {
  console.log(`uPlot loaded from ${src}`);
  return loadWhenFree(minHeap, "/js/bootstrap.bundle.5.2.3.min.js", "https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/js/bootstrap.bundle.min.js", "js", () => { window.bootstrap = bootstrap; });
})
.then(src => {
  console.log(`Bootstrap loaded from ${src}`);
  return waitForHeap(minHeap).then(() => import('/js/app.js?v=1'));
})
.then(() => {
  console.log("App loaded");
  if (window.__libsCallback) window.__libsCallback();
})
.catch(err => {
  console.error("Error loading JS libraries:", err);
});