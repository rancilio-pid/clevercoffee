const appCreatedEvent = new CustomEvent('appCreated')

const vueApp = Vue.createApp({
    data() {
        return {
            parameters: [],
            originalValues: {},
            parametersHelpTexts: [],
            isPostingForm: false,
            showPostSucceeded: false,
            filter: '',

            // Reboot notification
            showRebootBanner: false,
            changedRebootParams: [],

            // Config upload properties
            selectedFile: null,
            isUploading: false,
            uploadMessage: '',
            uploadSuccess: false,

            // Factory reset properties
            factoryResetMessage: '',
            factoryResetSuccess: false
        }
    },

    mounted() {
        // Get filter from URL parameter if available
        const urlParams = new URLSearchParams(window.location.search);
        let filter = urlParams.get('filter');

        // If no filter specified and we're on index page, use empty filter to get all parameters
        if (!filter && (window.location.pathname === '/' || window.location.pathname === '/index.html')) {
            filter = '';
        } else if (!filter) {
            filter = this.filter; // use default
        }

        this.filter = filter;

        // Fetch parameters with the determined filter
        this.fetchParameters(this.filter);
    },

    methods: {
        async fetchParameters(filter = '') {
            this.parameters = [];
            this.originalValues = {}; // Reset original values
            let offset = 0;
            const limit = 5;
            let moreData = true;

            while (moreData) {
                // Build URL with dynamic filter, offset, and limit
                let url = `/parameters?offset=${offset}&limit=${limit}`;

                if (filter) {
                    url += `&filter=${encodeURIComponent(filter)}`;
                }

                try {
                    const response = await fetch(url);
                    const json = await response.json();

                    if (!json.parameters || json.parameters.length === 0) {
                        moreData = false; // no more data
                        break;
                    }

                    // Append new parameters and store original values
                    json.parameters.forEach(param => {
                        this.parameters.push(param);
                        // Store a copy of the original value for change detection
                        this.originalValues[param.name] = param.value;
                    });

                    if (json.parameters.length < limit) {
                        moreData = false; // last page reached
                    }
                    else {
                        offset += limit; // increment offset for next batch
                    }
                }
                catch (err) {
                    console.error('Error fetching parameters:', err);
                    moreData = false;
                }
            }
        },

        postParameters() {
            // Only post parameters that are currently displayed (filtered parameters)
            const formBody = [];

            // Get currently displayed parameters from the computed property
            const displayedSections = this.parameterSectionsComputed;

            // Flatten all displayed parameters
            const displayedParameters = [];
            Object.values(displayedSections).forEach(section => {
                Object.values(section).forEach(group => {
                    group.forEach(param => {
                        if (param.show) {
                            displayedParameters.push(param);
                        }
                    });
                });
            });

            // Track which reboot-required parameters actually changed
            const rebootParamsChanged = displayedParameters
                .filter(param => {
                    if (!param.reboot) return false;

                    const originalValue = this.originalValues[param.name];
                    // Compare as strings to handle type coercion (e.g., "1" vs 1)
                    return String(param.value) !== String(originalValue);
                })
                .map(param => param.displayName);

            // Build form data from displayed parameters
            displayedParameters.forEach(param => {
                const key = param.name;
                const encodedValue = encodeURIComponent(param.value);
                formBody.push(key + "=" + encodedValue);
            });

            if (formBody.length === 0) {
                console.log("No parameters to save");
                return;
            }

            const formBodyString = formBody.join("&");

            const requestOptions = {
                method: "POST",
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                cache: 'no-cache',
                body: formBodyString
            };

            this.isPostingForm = true;

            const url = "/parameters";

            fetch(url, requestOptions)
                .then(response => {
                    if (!response.ok) {
                        throw new Error(`HTTP error! status: ${response.status}`);
                    }
                    return response.text();
                })
                .then(data => {
                    // Parameters saved successfully - now re-fetch to get updated show conditions
                    this.fetchParameters(this.filter);

                    // Show reboot banner only if reboot-required params actually changed
                    if (rebootParamsChanged.length > 0) {
                        this.changedRebootParams = rebootParamsChanged;
                        this.showRebootBanner = true;
                    }
                })
                .catch(err => {
                    console.error("Error saving parameters:", err);
                })
                .finally(() => {
                    this.isPostingForm = false;
                    this.showPostSucceeded = true;
                    setTimeout(() => {
                        this.showPostSucceeded = false;
                    }, 2000);
                });
        },

        fetchHelpText(paramName) {
            if (!(paramName in this.parametersHelpTexts)) {
                // Find the parameter to check if it requires reboot
                const param = this.parameters.find(p => p.name === paramName);
                const requiresReboot = param && param.reboot;

                fetch("/parameterHelp/?param="+paramName)
                    .then(response => response.json())
                    .then(data => {
                        let helpText = data['helpText'] || '';

                        if (requiresReboot) {
                            const rebootMsg = '<br><strong>Changes require a reboot</strong>';
                            helpText = helpText ? helpText + rebootMsg : rebootMsg.substring(4);
                        }

                        this.parametersHelpTexts[paramName] = helpText;
                    })
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
                9: 'System Settings',
                10: 'Other',
                11: 'OLED Display',
                12: 'Relays',
                13: 'Switches',
                14: 'LEDs',
                15: 'Sensors'
            }

            return sectionNames[sectionId]
        },

        // Extract group number from position (second digit)
        getGroupFromPosition(position) {
            return Math.floor((position % 100) / 10);
        },

        // Helper method to determine input type for parameters
        getInputType(param) {
            switch(param.type) {
                case 5: // enum
                    return 'select';
                case 4: // string
                    return 'text';
                case 0: // integer
                case 1: // uint8
                case 2: // double
                case 3: // float
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
                case 2: // double
                case 3: // float
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

                if (!response.ok) {
                    // Try to parse error message if possible
                    let errorMessage = 'Upload failed. Please try again.';

                    try {
                        const errorData = await response.json();

                        if (errorData.message) {
                            errorMessage = errorData.message;
                        }
                    } catch (jsonError) {
                        errorMessage = `Upload failed: ${response.status} ${response.statusText}`;
                    }

                    this.uploadMessage = errorMessage;
                    this.uploadSuccess = false;

                    await this.handlePostUploadRestart();

                    return;
                }

                let result;
                try {
                    result = await response.json();
                } catch (jsonError) {
                    // If JSON parsing fails but response was OK, assume success
                    this.uploadMessage = 'Configuration uploaded successfully!';
                    this.uploadSuccess = true;

                    await this.handlePostUploadRestart();

                    return;
                }

                this.uploadSuccess = result.success;
                this.uploadMessage = result.message || (result.success ? 'Configuration uploaded successfully!' : 'Configuration validation failed.');

                // Always restart after config upload attempt
                await this.handlePostUploadRestart();

            } catch (error) {
                console.error('Upload error:', error);

                if (error.name === 'TypeError' && error.message.includes('fetch')) {
                    this.uploadMessage = 'Network error: Could not connect to device. Please try again.';
                } else if (error.name === 'AbortError') {
                    this.uploadMessage = 'Upload was cancelled or timed out. Please try again.';
                } else {
                    this.uploadMessage = 'Upload failed due to an unexpected error. Please try again.';
                }

                this.uploadSuccess = false;
            } finally {
                this.isUploading = false;
            }
        },

        async handlePostUploadRestart() {
            // Give user time to read the message
            await new Promise(resolve => setTimeout(resolve, 2000));

            this.uploadMessage += ' Restarting machine...';

            await new Promise(resolve => setTimeout(resolve, 1000));

            try {
                await this.restartMachine();
            } catch (error) {
                console.log('Machine restarting...');
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

        async confirmFactoryReset() {
            if (!window.confirm("Are you sure you want to reset the config to defaults and restart the ESP? This can't be undone.")) {
                return;
            }

            this.factoryResetMessage = 'Factory reset initiated. Machine is restarting...';
            this.factoryResetSuccess = true;

            try {
                await fetch("/factoryreset", { method: "POST" });
            } catch (err) {
                console.log('Machine restarting after factory reset...');
            }
        },

        dismissRebootBanner() {
            this.showRebootBanner = false;
            this.changedRebootParams = [];
        },

        async restartMachine() {
            try {
                await fetch('/restart', { method: 'POST' });
                alert('Machine is restarting...');
            } catch (e) {
                // Expected - machine is restarting
            }
        },

        toggleFunction(endpoint, paramName, param, targetElement) {
            const formData = new FormData();
            formData.append(`var${paramName}`, param.value === 1 ? '0' : '1');

            fetch(endpoint, {
                method: 'POST',
                body: formData
            })
                .then(response => {
                    if (response.ok) {
                        // Update the parameter value
                        param.value = param.value === 1 ? 0 : 1;
                    } else {
                        console.error('Toggle failed');
                        setTimeout(() => {
                            if (targetElement) {
                                targetElement.checked = param.value === 1;
                            }
                        }, 100);
                    }
                })
                .catch(error => {
                    console.error('Toggle error:', error);
                    setTimeout(() => {
                        if (targetElement) {
                            targetElement.checked = param.value === 1;
                        }
                    }, 100);
                });
        },

        confirmCalibration() {
            if (confirm('Are you sure you want to start the scale calibration?')) {
                this.executeAction('/toggleScaleCalibration', 'CALIBRATION_ON');
            }
        },

        executeAction(endpoint, paramName) {
            fetch(endpoint, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: `var${paramName}=1`
            });
        },

    },

    computed: {
        parameterSectionsComputed() {
            const excludedSections = [10]
            const filteredParameters = this.parameters.filter(param => !excludedSections.includes(param.section))

            // First group by section
            const sections = groupBy(filteredParameters, "section")

            // Then group each section by the group number (second digit of position)
            const result = {}
            Object.keys(sections).forEach(sectionKey => {
                result[sectionKey] = groupBy(sections[sectionKey], param => this.getGroupFromPosition(param.position))
            })

            return result
        }
    }
})

// Register VueNumberInput component globally if it exists
if (typeof VueNumberInput !== 'undefined') {
    vueApp.component(VueNumberInput.name, VueNumberInput);
}

window.vueApp = vueApp
window.dispatchEvent(appCreatedEvent)
window.appCreated = true

/**
 * Takes an array of objects and returns an object of arrays where the value of key is the same
 */
function groupBy(array, key) {
    const result = {}

    array.forEach(item => {
        const groupKey = typeof key === 'function' ? key(item) : item[key]

        if (!result[groupKey]) {
            result[groupKey] = []
        }

        result[groupKey].push(item)
    })

    return result
}

// Init Bootstrap Popovers
document.querySelector('body').addEventListener('click', function (e) {
    //if click was not on an opened popover (ignore those)
    if (!e.target.classList.contains("popover-header")) {
        //close popovers when clicking elsewhere
        if (e.target.parentElement.getAttribute("data-bs-toggle") !== "popover") {
            document.querySelectorAll('[data-bs-toggle="popover"]').forEach(function(el) {
                const popover = bootstrap.Popover.getInstance(el);

                if (popover !== null) {
                    popover.hide();
                }
            });
        }
        else {
            e.preventDefault();

            // create new popover
            const popover = bootstrap.Popover.getOrCreateInstance(e.target.parentElement);
            popover.show();
        }
    }
});
