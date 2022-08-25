const appCreatedEvent = new Event('appCreated')

const vueApp = Vue.createApp({
    data() {
        return {
            parameters: [],
            parameterSections: [],
            parametersHelpTexts: [],
            isPostingForm: false
        }
    },
    methods: {
        fetchParameters() {
            fetch("/parameters")
                .then(response => response.json())
                .then(data => {
                    this.parameters = data
                    this.getParameterSections()
                })
                .catch(err => console.log(err.messages))
        },
        postParameters() {
            //post parameter array same as if it was posted from a form
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
                .catch(err => console.log(err.messages))
                .finally(() => {
                    this.isPostingForm = false
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
        getParameterSections() {
            var sections = groupBy(this.parameters, "section")
            this.parameterSections = sections
        },
        sectionName(sectionId) {
            const sectionNames = {
                0: 'PID Parameters',
                1: 'Temperature and Preinfusion',
                2: 'Brew Detection and Brew PID Parameters'
            }
            return sectionNames[sectionId]
        }
    },
    mounted() {
        this.fetchParameters()
    }
})

window.vueApp = vueApp
window.dispatchEvent(appCreatedEvent)

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