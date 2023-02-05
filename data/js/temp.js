//TODO: this might have a short race condition where readyState is complete but load has not yet fired
if (document.readyState === 'complete') {
    getTimeseries()
} else {
    window.addEventListener('load', getTimeseries)
}

const maxRange = 3600

var curTempVals = []
var targetTempVals = []
var heaterPowerVals = []
var tempDates = []
var heaterDates = []

var chartDiv = 'chart-temperature'
var heaterDiv = 'chart-heater'

let uplotTemp = null;
let uplotHeater = null;

function createTempData(jsonValue, isSingleValue=false) {
    if (isSingleValue) {
        const curTempKey = "currentTemp"
        const targetTempKey = "targetTemp"

        //add new value to data lists
        var date = new Date()
        //console.log(date)
        tempDates.push(date)

        var curTemp = jsonValue[curTempKey]
        //console.log(curTemp)
        curTempVals.push(curTemp)

        var targetTemp = jsonValue[targetTempKey]
        //console.log(targetTemp)
        targetTempVals.push(targetTemp)
    } else {
        const curTempKey = "currentTemps"
        const targetTempKey = "targetTemps"

        //set data lists to values from history json
        var curTemp = jsonValue[curTempKey]
        //console.log(curTemp)
        curTempVals.length = 0  //reset existing list
        curTempVals.push(...curTemp)

        var targetTemp = jsonValue[targetTempKey]
        //console.log(targetTemp)
        targetTempVals.length = 0
        targetTempVals.push(...targetTemp)
        
        //create dates for all history values (3 seconds between each value)
        tempDates.length = 0
        for (let i=curTempVals.length; i>0; i--) {
            var date = new Date()
            date.setSeconds(date.getSeconds()-3*i)
            tempDates.push(date)
        }
    }

    //reduce data if we have too much
    if (tempDates.length > maxRange) {
        tempDates.splice(0, tempDates.length - maxRange)
        curTempVals.splice(0, curTempVals.length - maxRange)
        targetTempVals.splice(0, targetTempVals.length - maxRange)
    }

    //use data lists to create data array for uPlot
    let data = [
        Array(tempDates.length),
        Array(curTempVals.length),
        Array(targetTempVals.length),
    ];

    for (let i = 0; i < curTempVals.length; i++) {
        data[0][i] = tempDates[i].getTime() / 1000
        data[1][i] = curTempVals[i]
        data[2][i] = targetTempVals[i]
    }

    return data
}

function createHeaterData(jsonValue, isSingleValue=false) {
    if (isSingleValue) {
        const heaterPowerKey = "heaterPower"

        //add new value to data lists
        var date = new Date()
        heaterDates.push(date)

        var heaterPower = jsonValue[heaterPowerKey]
        heaterPowerVals.push(heaterPower)
    } else {
        const heaterPowerKey = "heaterPowers"

        //set data lists to values from history json
        var heaterPower = jsonValue[heaterPowerKey]
        //console.log(heaterPower)
        heaterPowerVals.length = 0
        heaterPowerVals.push(...heaterPower)
        
        //create dates for all history values (3 seconds between each value)
        heaterDates.length = 0
        for (let i=heaterPowerVals.length; i>0; i--) {
            var date = new Date()
            date.setSeconds(date.getSeconds()-3*i)
            heaterDates.push(date)
        }
    }

    //reduce data if we have too much
    if (heaterDates.length > maxRange) {
        heaterDates.splice(0, heaterDates.length - maxRange)
        heaterPowerVals.splice(0, heaterPowerVals.length - maxRange)
    }

    //use data lists to create data array for uPlot
    let data = [
        Array(heaterDates.length),
        Array(heaterPowerVals.length),
    ];

    for (let i = 0; i < heaterPowerVals.length; i++) {
        data[0][i] = heaterDates[i].getTime() / 1000
        data[1][i] = heaterPowerVals[i]
    }

    return data
}

function sliceData(data, start, end) {
    let d = [];

    for (let i = 0; i < data.length; i++)
        d.push(data[i].slice(start, end));

    return d;
}

const lineInterpolations = {
    linear:     0,
    stepAfter:  1,
    stepBefore: 2,
    spline:     3,
};

const tzdateOptions = {
    hour: '2-digit', minute: '2-digit', second: '2-digit',
};

function makeTempChart(data) {
    const opts = {
        title: "Temperature History",
        //id: "chart1",
        //class: "my-chart",
        ...getSize(chartDiv),
        //width: window.innerWidth * 0.9,
        //height: 400,
        tzDate: ts => uPlot.tzDate(new Date(ts * 1e3), 'Europe/Berlin'),
        series: [
            {
                value: "{YYYY}-{MM}-{DD} {HH}:{mm}:{ss}"
            },
            {
                label: "Current Temperature",
                scale: "C",
                value: (u, v) => v == null ? null : v,
                show: true,
                stroke: "#008080",
                fill: "#00808010",
                lineInterpolation: lineInterpolations.spline,
                points: { show: false },
            },
            {
                label: "Target Temperature",
                scale: "C",
                value: (u, v) => v == null ? null : v,
                stroke: "#9932CC",
                fill: "#9932CC10",
                lineInterpolation: lineInterpolations.spline,
                show: true,
                points: { show: false },
            }
        ],
        axes: [
            {
                values: (u, vals, space) => vals.map(v => uPlot.tzDate(new Date(v * 1e3), 'Europe/Berlin').toLocaleString("de-DE", tzdateOptions)),
                // [0]:   minimum num secs in found axis split (tick incr)
                // [1]:   default tick format
                // [2-7]: rollover tick formats
                // [8]:   mode: 0: replace [1] -> [2-7], 1: concat [1] + [2-7]
                /*
                values: [
                    // tick incr        default           year                             month    day                        hour     min             sec       mode
                    [3600 * 24 * 365,   "{YYYY}",         null,                            null,    null,                      null,    null,           null,        1],
                    [3600 * 24 * 28,    "{MMM}",          "\n{YYYY}",                      null,    null,                      null,    null,           null,        1],
                    [3600 * 24,         "{M}/{D}",        "\n{YYYY}",                      null,    null,                      null,    null,           null,        1],
                    [3600,              "{hh}",           "\n{M}/{D}/{YY}",                null,    "\n{M}/{D}",               null,    null,           null,        1],
                    [60,                "{hh}:{mm}",      "\n{M}/{D}/{YY}",                null,    "\n{M}/{D}",               null,    null,           null,        1],
                    [1,                 ":{ss}",          "\n{M}/{D}/{YY} {hh}:{mm}",      null,    "\n{M}/{D} {hh}:{mm}",     null,    "\n{hh}:{mm}",  null,        1],
                    [0.001,             ":{ss}.{fff}",    "\n{M}/{D}/{YY} {hh}:{mm}",      null,    "\n{M}/{D} {hh}:{mm}",     null,    "\n{hh}:{mm}",  null,        1],
                ],
                */
            },
            {
                scale: 'C',
                values: (u, vals, space) => vals.map(v => +v + "C"),
            }
        ],
    };
    
    uplotTemp = new uPlot(opts, sliceData(data, 0, data.length), document.getElementById(chartDiv));
}

function makeHeaterChart(data) {
    const opts = {
        title: "Heater Power History",
        //id: "chart1",
        //class: "my-chart",
        ...getSize(heaterDiv),
        //width: window.innerWidth * 0.9,
        //height: 400,
        tzDate: ts => uPlot.tzDate(new Date(ts * 1e3), 'Europe/Berlin'),
        series: [
            {
                value: "{YYYY}-{MM}-{DD} {HH}:{mm}:{ss}"
            },
            {
                label: "Heater Power",
                scale: "W",
                value: (u, v) => v == null ? null : v.toFixed(1) + " W",
                stroke: "#778899",
                fill: "#77889910",
                lineInterpolation: lineInterpolations.spline,
                show: true,
                points: { show: false },
            }
        ],
        axes: [
            {
                values: (u, vals, space) => vals.map(v => uPlot.tzDate(new Date(v * 1e3), 'Europe/Berlin').toLocaleString("de-DE", tzdateOptions)),
            },
            {
                side: 0,
                scale: 'W',
                values: (u, vals, space) => vals.map(v => +v.toFixed(1) + " W"),
            },
        ],
    };
    
    uplotHeater = new uPlot(opts, sliceData(data, 0, data.length), document.getElementById(heaterDiv));
}

//append single plot data values
function addPlotData(jsonValue) {    
    let tempData = createTempData(jsonValue, true);
    if (uplotTemp !== null) {
        uplotTemp.setData(tempData);
    }

    let heaterData = createHeaterData(jsonValue, true);
    if (uplotHeater !== null) {
        uplotHeater.setData(heaterData);
    }
}

function getSize(selector) {
    return {
        width: document.getElementById(selector).offsetWidth,
        height: 400 //document.getElementById(selector).offsetHeight,
    }
}

//resize plots when window is resized
window.addEventListener("resize", e => {
    if (uplotTemp !== null) {
        uplotTemp.setSize(getSize(chartDiv));
    }
    if (uplotHeater !== null) {
        uplotHeater.setSize(getSize(heaterDiv));
    }
});


// get data from server
function getTimeseries() {
    var xhr = new XMLHttpRequest()

    xhr.onload = (e) => {
        if (xhr.readyState === 4 && xhr.status === 200) {
            var tempHistory = JSON.parse(xhr.responseText)
            let tempData = createTempData(tempHistory);
            let heaterData = createHeaterData(tempHistory);
            setTimeout(() => {
                makeTempChart(tempData);
                makeHeaterChart(heaterData);
            }, 0);
        }
    }

    xhr.open("GET", "/timeseries", true)
    xhr.send()
}

// listen to events to update data from endpoints
if (!!window.EventSource) {
    var source = new EventSource('/events')

    source.addEventListener(
        'open',
        function (e) {
            console.log("Event source connected")
        },
        false
    )

    source.addEventListener(
        'error',
        function (e) {
            if (e.target.readyState != EventSource.OPEN) {
                console.log("Events source disconnected")
            }
        },
        false
    )

    source.addEventListener(
        'new_temps',
        function (e) {
            var myObj = JSON.parse(e.data)
            //console.log("new_temps", e.data)
            //console.log(myObj)

            // add new data to existing for plotting            
            addPlotData(myObj)

            // update current temp value on index page
            document.getElementById("varTEMP").innerText = myObj["currentTemp"].toFixed(1)
        },
        false
    )
}