const appCreatedEvent = new Event('appCreated')

const vueApp = Vue.createApp({
    data() {
        return {
            parameters: [],
            parametersHelpTexts: [],
            isPostingForm: false,
            showPostSucceeded: false
        }
    },
    methods: {
        fetchParameters() {
            fetch("/parameters")
                .then(response => response.json())
                .then(data => {
                    this.parameters = data.sort((a,b) => a["position"] - b["position"])
                })
                .catch(err => console.log(err.messages))
        },
        postParameters() {
            //post parameter array the same as if it was posted from a form (the values are already updated
            //from the v-model bindings)
            var formBody = [];
            this.parameters.forEach(param => {
                var encodedKey = encodeURIComponent(param.name);
                var encodedValue = encodeURIComponent(param.value);
                formBody.push(encodedKey + "=" + encodedValue);
            });
            formBody = formBody.join("&");

            const requestOptions = {
                method: "POST",
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                cache: 'no-cache',
                body: formBody
            };
            this.isPostingForm = true
            fetch("/parameters", requestOptions)
                .then(response => 0)
                .catch(err => {
                    console.log(err.messages)
                    //TODO: show (red) error symbol
                })
                .finally(() => {
                    this.isPostingForm = false
                    //show checkmark, hide after timeout
                    this.showPostSucceeded = true
                    setTimeout(() => {
                        this.showPostSucceeded = false
                    }, 3000)

                    //refresh parameters to be shown
                    this.fetchParameters()
                })
        },
        fetchHelpText(paramName) {
            if (!(paramName in this.parametersHelpTexts)) {
                fetch("/parameterHelp/?param="+paramName)
                    .then(response => response.json())
                    .then(data => { this.parametersHelpTexts[paramName] = data['helpText'] })
            }
        },
        sectionName(sectionId) {
            const sectionNames = {
                0: 'PID Parameters',
                1: 'Temperature and Preinfusion',
                2: 'Brew Detection and Brew PID Parameters',
                3: 'Power Settings',
                3: 'Scale Parameters'
            }
            return sectionNames[sectionId]
        }
    },
    computed: {
        parameterSections() {
            return groupBy(this.parameters, "section")
        }
    },
    mounted() {
        this.fetchParameters()
    }
})

window.vueApp = vueApp
window.dispatchEvent(appCreatedEvent)
window.appCreated = true

/**
* Takes an array of objects and returns an object of arrays where the value of key is the same
*/
function groupBy(array, key) {
    const result = {}
    array.forEach(item => {
        if (!result[item[key]]) {
            result[item[key]] = []
    }
        result[item[key]].push(item)
    })
    return result
}

//Init Bootstrap Popovers
document.querySelector('body').addEventListener('click', function (e) {
    //if click was not on an opened popover (ignore those)
    if (!e.target.classList.contains("popover-header")) {
        //close popovers when clicking elsewhere
        if (e.target.parentElement.getAttribute("data-bs-toggle") != "popover") {
            document.querySelectorAll('[data-bs-toggle="popover"]').forEach(function(el) {
                var popover = bootstrap.Popover.getInstance(el);
                if (popover != null) {
                    popover.hide();
                }
            });
        } else {
            e.preventDefault();
            //create new popover
            var popover = bootstrap.Popover.getOrCreateInstance(e.target.parentElement);
            popover.show();
        }
    }
});
