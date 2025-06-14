
const appCreatedEvent = new Event('appCreated')

const vueApp = Vue.createApp({
    data() {
        return {
            parameters: [],
            parametersHelpTexts: [],
            isPostingForm: false,
            showPostSucceeded: false,

            // Config upload properties
            selectedFile: null,
            isUploading: false,
            uploadMessage: '',
            uploadSuccess: false
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
            // post parameter array the same as if it was posted from a form (the values are already updated
            // from the v-model bindings)
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
                    // TODO: show (red) error symbol
                })
                .finally(() => {
                    this.isPostingForm = false
                    // show checkmark, hide after timeout
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
                1: 'Temperature',
                2: 'Brew PID Parameters',
                3: 'Brew Control',
                4: 'Scale Parameters',
                5: 'Display Settings',
                6: 'Maintenance',
                7: 'Power Settings',
                8: 'MQTT Settings',
                9: 'System Settings'
            }

            return sectionNames[sectionId]
        },

        // Helper method to determine input type for parameters
        getInputType(param) {
            switch(param.type) {
                case 6: // enum
                    return 'select';
                case 4: // string
                    return 'text';
                case 0: // integer
                case 1: // uint8
                case 2: // float/double
                case 3: // doubletime
                    return 'number';
                default:
                    return 'text';
            }
        },

        // Helper method to get step value for number inputs
        getNumberStep(param) {
            switch(param.type) {
                case 0: // integer
                case 1: // uint8
                    return '1';
                case 2: // float/double
                case 3: // doubletime
                    return '0.01';
                default:
                    return '1';
            }
        },

        // Helper method to check if parameter is a boolean (displayed as checkbox)
        isBoolean(param) {
            // Type 1 is uint8, and if min=0 max=1, it's a boolean checkbox
            return param.type === 1 && param.min === 0 && param.max === 1;
        },

        confirmSubmission() {
            if (confirm('Are you sure you want to start the scale calibration?')) {
                const requestOptions = {
                    method: "POST",
                    cache: 'no-cache'
                };

                fetch("/toggleScaleCalibration", requestOptions)
            }
        },

        async confirmReset() {
            const confirmed = window.confirm(
                "Are you sure you want to reset the WiFi settings?\n\nThis will erase saved credentials and restart the device."
            );

            if (!confirmed) return;

            try {
                const response = await fetch("/wifireset", { method: "POST" });
                const text = await response.text();
                alert(text);
            } catch (err) {
                alert("Reset failed: " + err.message);
            }
        },

        // Config upload methods
        handleFileSelect(event) {
            const file = event.target.files[0];
            this.selectedFile = file;
            this.uploadMessage = '';

            if (file) {
                // Validate file
                if (!file.name.toLowerCase().endsWith('.json')) {
                    this.uploadMessage = 'Please select a valid JSON configuration file.';
                    this.uploadSuccess = false;
                    this.selectedFile = null;
                    return;
                }

                // Check file size (limit to 50KB)
                const maxSize = 50 * 1024;
                if (file.size > maxSize) {
                    this.uploadMessage = 'Configuration file is too large. Maximum size is 50KB.';
                    this.uploadSuccess = false;
                    this.selectedFile = null;
                    return;
                }

                this.uploadMessage = `Selected: ${file.name} (${this.formatFileSize(file.size)})`;
                this.uploadSuccess = true;
            }
        },

        async uploadConfig() {
            if (!this.selectedFile) {
                this.uploadMessage = 'Please select a configuration file first.';
                this.uploadSuccess = false;
                return;
            }

            this.isUploading = true;
            this.uploadMessage = 'Uploading configuration...';

            try {
                const formData = new FormData();
                formData.append('config', this.selectedFile);

                const response = await fetch('/upload/config', {
                    method: 'POST',
                    body: formData
                });

                if (response.ok) {
                    this.uploadMessage = 'Configuration uploaded successfully! Restarting device...';
                    this.uploadSuccess = true;

                    // Wait a moment, then trigger restart
                    setTimeout(async () => {
                        try {
                            await fetch('/restart', { method: 'POST' });
                        } catch (e) {
                            // Expected - machine is restarting
                        }

                        this.uploadMessage = 'Machine is restarting. Please wait...';
                    }, 1500);
                } else {
                    this.uploadMessage = 'Upload failed. Please try again.';
                    this.uploadSuccess = false;
                }
            } catch (error) {
                this.uploadMessage = 'Upload failed due to network error. Please try again.';
                this.uploadSuccess = false;
                console.error('Upload error:', error);
            } finally {
                this.isUploading = false;
            }
        },

        formatFileSize(bytes) {
            if (bytes === 0) return '0 Bytes';

            const k = 1024;
            const sizes = ['Bytes', 'KB', 'MB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));

            return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
        },

        confirmRestart() {
            if (confirm('Are you sure you want to restart your machine?')) {
                this.restartMachine();
            }
        },

        async restartMachine() {
            try {
                await fetch('/restart', { method: 'POST' });
                alert('Machine is restarting...');
            } catch (e) {
                // Expected - machine is restarting
            }
        }
    },

    computed: {
        parameterSections() {
            const excludedSections = [10] // Don't show sOtherSection
            const filteredParameters = this.parameters.filter(param => !excludedSections.includes(param.section))
            return groupBy(filteredParameters, "section")
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

// Init Bootstrap Popovers
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
        }
        else {
            e.preventDefault();

            // create new popover
            var popover = bootstrap.Popover.getOrCreateInstance(e.target.parentElement);
            popover.show();
        }
    }
});
