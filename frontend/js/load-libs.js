// load-libs.js

function loadLib(localUrl, cdnUrl, type="js", globalAssign=null) {
  return new Promise((resolve, reject) => {
    let finished = false;

    function inject(url, isLocal) {
      let el;
      if(type === "js") {
        el = document.createElement("script");
        el.src = url;
        el.defer = true;
      } else if(type === "css") {
        el = document.createElement("link");
        el.rel = "stylesheet";
        el.href = url;
      }

      el.onload = () => {
        if(!finished){
          finished = true;
          if(globalAssign && type==="js") {
            globalAssign();
          }
          resolve(isLocal ? "local" : "cdn");
        }
      };

      el.onerror = () => { 
        if(isLocal) reject(); // fail only if local fails
      };

      document.head.appendChild(el);
      return el;
    }

    let localEl = inject(localUrl, true);

    if(cdnUrl){
      let cdnEl = inject(cdnUrl, false);
      cdnEl.onload = () => {
        if(!finished){
          finished = true;
          if(localEl.parentNode) localEl.parentNode.removeChild(localEl);
          if(globalAssign && type==="js") {
            globalAssign();
          }
          resolve("cdn");
        }
      };
    }
  });
}

// --------------------------
// Load CSS in sequence
// --------------------------
loadLib("/css/fontawesome-6.2.1.min.css", "https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.2.1/css/all.min.css", "css")
.then(src => {
  console.log(`FontAwesome CSS loaded from ${src}`);
  return loadLib("/css/bootstrap-5.2.3.min.css", "https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/css/bootstrap.min.css", "css");
})
.then(src => {
  console.log(`Bootstrap CSS loaded from ${src}`);
  return loadLib("/css/uPlot.min.css", "https://cdn.jsdelivr.net/npm/uplot@1.6.24/dist/uPlot.min.css", "css");
})
.then(src => {
  console.log(`uPlot CSS loaded from ${src}`);
// --------------------------
// Load JS in sequence
// --------------------------
  return loadLib("/js/vue.3.2.47.min.js", "https://cdn.jsdelivr.net/npm/vue@3.2.47/dist/vue.global.prod.min.js", "js", () => { window.Vue = Vue; });
})
.then(src => {
  console.log(`Vue loaded from ${src}`);
  return loadLib("/js/vue-number-input.min.js", "https://unpkg.com/@chenfengyuan/vue-number-input@2.0.1/dist/vue-number-input.min.js", "js", () => { window.VueNumberInput = VueNumberInput; });
})
.then(src => {
  console.log(`VueNumberInput loaded from ${src}`);
  return loadLib("/js/uPlot.1.6.28.min.js", "https://cdn.jsdelivr.net/npm/uplot@1.6.28/dist/uPlot.iife.min.js", "js", () => { window.uPlot = uPlot; });
})
.then(src => {
  console.log(`uPlot loaded from ${src}`);
  return loadLib("/js/bootstrap.bundle.5.2.3.min.js", "https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/js/bootstrap.bundle.min.js", "js", () => { window.bootstrap = bootstrap; });
})
.then(src => {
  console.log(`Bootstrap loaded from ${src}`);
  return import('/js/app.js?v=1');
})
.then(() => {
  console.log("App loaded");
})
.catch(err => {
  console.error("Error loading JS libraries:", err);
});