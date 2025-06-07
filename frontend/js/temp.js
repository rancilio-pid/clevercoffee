// TODO: this might have a short race condition where readyState is complete but load has not yet fired
if (document.readyState === 'complete') {
    getTimeseries()
}
else {
    window.addEventListener('load', getTimeseries)
}

const maxValues = 600          // max number of data points to keep in memory
const updateInterval = 1000    // expected ms between updates (from event source with new values)

var curTempVals = []
var targetTempVals = []
var heaterPowerVals = []
var tempDates = []
var heaterDates = []

var chartDiv = 'chart-temperature'
var heaterDiv = 'chart-heater'

let uplotTemp = null;
let uplotHeater = null;

function addTempData(jsonValue, isSingleValue=false) {
    // add new value(s) to global data arrays and return in a
    // format that uPlot can use

    if (isSingleValue) {
        const curTempKey = "currentTemp"
        const targetTempKey = "targetTemp"

        // add new value to data lists
        var date = new Date()
        tempDates.push(date)

        var curTemp = jsonValue[curTempKey]
        curTempVals.push(curTemp)

        var targetTemp = jsonValue[targetTempKey]
        targetTempVals.push(targetTemp)
    }
    else {
        const curTempKey = "currentTemps"
        const targetTempKey = "targetTemps"

        // set data lists to values from history json
        var curTemp = jsonValue[curTempKey]
        curTempVals.length = 0  //reset existing list
        curTempVals.push(...curTemp)

        var targetTemp = jsonValue[targetTempKey]
        targetTempVals.length = 0
        targetTempVals.push(...targetTemp)
        
        // create dates for all history values (3 seconds between each value)
        tempDates.length = 0
        
        for (let i=curTempVals.length; i>0; i--) {
            var date = new Date()
            date.setSeconds(date.getSeconds()-3*i)
            tempDates.push(date)
        }
    }

    // reduce data if we have too much (sliding window after that amount)
    if (tempDates.length > maxValues) {
        tempDates.splice(0, tempDates.length - maxValues)
        curTempVals.splice(0, curTempVals.length - maxValues)
        targetTempVals.splice(0, targetTempVals.length - maxValues)
    }

    // use data lists to create data array for uPlot
    let data = [
        Array(tempDates.length),
        Array(curTempVals.length),
        Array(targetTempVals.length),
    ];

    for (let i = 0; i < curTempVals.length; i++) {
        data[0][i] = tempDates[i].getTime() / updateInterval 
        data[1][i] = curTempVals[i]
        data[2][i] = targetTempVals[i]
    }

    return data
}

function addHeaterData(jsonValue, isSingleValue=false) {
    if (isSingleValue) {
        const heaterPowerKey = "heaterPower"

        // add new value to data lists
        var date = new Date()
        heaterDates.push(date)

        var heaterPower = jsonValue[heaterPowerKey]
        heaterPowerVals.push(heaterPower)
    }
    else {
        const heaterPowerKey = "heaterPowers"

        // set data lists to values from history json
        var heaterPower = jsonValue[heaterPowerKey]
        heaterPowerVals.length = 0
        heaterPowerVals.push(...heaterPower)
        
        // create dates for all history values (3 seconds between each value)
        heaterDates.length = 0
        
        for (let i=heaterPowerVals.length; i>0; i--) {
            var date = new Date()
            date.setSeconds(date.getSeconds()-3*i)
            heaterDates.push(date)
        }
    }

    // reduce data if we have too many values (sliding window after that amount)
    if (heaterDates.length > maxValues) {
        heaterDates.splice(0, heaterDates.length - maxValues)
        heaterPowerVals.splice(0, heaterPowerVals.length - maxValues)
    }

    // use data lists to create data array for uPlot
    let data = [
        Array(heaterDates.length),
        Array(heaterPowerVals.length),
    ];

    for (let i = 0; i < heaterPowerVals.length; i++) {
        data[0][i] = heaterDates[i].getTime() / updateInterval 
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

const drawStyles = {
    line:      0,
    bars:      1,
    points:    2,
    barsLeft:  3,
    barsRight: 4,
};

const { linear, stepped, bars, spline } = uPlot.paths;
const _linear       = linear();
const _spline       = spline();

function pathRenderer(u, seriesIdx, idx0, idx1, extendGap, buildClip) {
    let s = u.series[seriesIdx];
    let style = s.drawStyle;
    let interp = s.lineInterpolation;

    let renderer = null;

    if (style == drawStyles.line) {
        if (interp == lineInterpolations.linear) {
            renderer = _linear
        }
        else if (interp == lineInterpolations.spline) {
            renderer = _spline
        }
    }
    else if (style == drawStyles.bars) {
        renderer =_bars60_100
    }
    else if (style == drawStyles.barsLeft) {
        renderer = _bars100Left
    }
    else if (style == drawStyles.barsRight) { 
        renderer = _bars100Right
    }
    else if (style == drawStyles.points) {
        renderer = () => null
    } 

    return renderer(u, seriesIdx, idx0, idx1, extendGap, buildClip);
};

const tzdateOptions = {
    hour: '2-digit', minute: '2-digit', second: '2-digit',
};

function makeTempChart(data) {
    const opts = {
        title: "Temperature History",
        ...getSize(chartDiv),
        tzDate: ts => uPlot.tzDate(new Date(ts * 1e3), 'Europe/Berlin'),
        series: [
            {
                value: "{YYYY}-{MM}-{DD} {HH}:{mm}:{ss}"
            },
            Object.assign({
                label: "Current Temperature",
                scale: "C",
                value: (u, v) => v == null ? null : v.toFixed(1) + " °C",
                show: true,
                stroke: "#008080",
                fill: "#00808010",
                width: 2,            
                points: { show: false },
                drawStyle: drawStyles.line,
				lineInterpolation: lineInterpolations.spline,
                paths: pathRenderer,
            }),
            
            {
                label: "Target Temperature",
                scale: "C",
                value: (u, v) => v == null ? null : v.toFixed(1) + " °C",
                stroke: "#9932CC",
                fill: "#9932CC10",
                width: 2,
                show: true,
                points: { show: false },
                drawStyle: drawStyles.line,
                lineInterpolation: lineInterpolations.spline,
                paths: pathRenderer,
            }
        ],
        axes: [
            {
                values: (u, vals, space) => vals.map(v => uPlot.tzDate(new Date(v * 1e3), 'Europe/Berlin').toLocaleString("de-DE", tzdateOptions)),
            },
            {
                scale: 'C',
                values: (u, vals, space) => vals.map(v => +v + "°C"),
            }
        ],
    };
    
    uplotTemp = new uPlot(opts, sliceData(data, 0, data.length), document.getElementById(chartDiv));
}

function makeHeaterChart(data) {
    const opts = {
        title: "Heater Power History",
        ...getSize(heaterDiv),
        tzDate: ts => uPlot.tzDate(new Date(ts * 1e3), 'Europe/Berlin'),
        scales: {
            "%": {
                auto: false,
                range: [0, 105],
            }
        },
        series: [
            {
                value: "{YYYY}-{MM}-{DD} {HH}:{mm}:{ss}"
            },
            {
                label: "Heater Power",
                scale: "%",
                value: (u, v) => v == null ? null : v.toFixed(0) + "%",
                stroke: "#778899",
                fill: "#77889910",
                width: 2,
                show: true,
                points: { show: false },
                drawStyle: drawStyles.line,
                lineInterpolation: lineInterpolations.spline,
                paths: pathRenderer,
            }
        ],
        axes: [
            {
                values: (u, vals, space) => vals.map(v => uPlot.tzDate(new Date(v * 1e3), 'Europe/Berlin').toLocaleString("de-DE", tzdateOptions)),
            },
            {
                side: 3,
                scale: '%',
                values: (u, vals, space) => vals.map(v => +v.toFixed(0) + "%"),
            },
        ],
    };
    
    uplotHeater = new uPlot(opts, sliceData(data, 0, data.length), document.getElementById(heaterDiv));
}

// append single historic values
function addPlotData(jsonValue) {    
    function addData(data, u) {
        let isTempZoomed = u.scales.x.min != u.data[0][0] || u.scales.x.max != u.data[0][u.data[0].length-1];

        if (isTempZoomed) {
            let tempXScaleMinMax = [u.scales.x.min, u.scales.x.max]            
            // add data but don't autoscale
            u.setData(data, false);
            // move the zoomed area one value to the right so the window stays the same
            u.setScale('x', {min: tempXScaleMinMax[0]+1, max: tempXScaleMinMax[1]+1});
        }
        else {
            // add data and autoscale (including new data)
            u.setData(data);
        }
    }

    let tempData = addTempData(jsonValue, true);

    if (uplotTemp !== null) {
        addData(tempData, uplotTemp)
    }

    let heaterData = addHeaterData(jsonValue, true);
    if (uplotHeater !== null) {
        addData(heaterData, uplotHeater)
    }
}

function getSize(selector) {
    return {
        width: document.getElementById(selector).offsetWidth,
        height: 400
    }
}

// resize plots when window is resized
window.addEventListener("resize", e => {
    if (uplotTemp !== null) {
        uplotTemp.setSize(getSize(chartDiv));
    }
    if (uplotHeater !== null) {
        uplotHeater.setSize(getSize(heaterDiv));
    }
});


// get initial history data from server
function getTimeseries() {
    var xhr = new XMLHttpRequest()

    xhr.onload = (e) => {
        if (xhr.readyState === 4 && xhr.status === 200) {
            var tempHistory = JSON.parse(xhr.responseText)
            let tempData = addTempData(tempHistory);
            let heaterData = addHeaterData(tempHistory);
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
            
            // add new data to existing for plotting            
            addPlotData(myObj)

            // update current temp value on index page
            document.getElementById("varTEMP").innerText = myObj["currentTemp"].toFixed(1)
        },
        false
    )
}