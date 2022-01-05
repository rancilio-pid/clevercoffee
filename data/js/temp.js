window.addEventListener('load', getTemperatures);

var curTempVals = [];
var targetTempVals = [];
var dates = [];

var chartDiv = 'chart-temperature';

var curTempTrace = {
  type: "scatter",
  mode: "lines",
  fill: 'tozeroy',
  name: 'Current',
  x: dates,
  y: curTempVals,
  line: {
    color: '#008080', 
    shape: 'spline'
  }
}

var targetTempTrace = {
  type: "scatter",
  mode: "lines",
  fill: 'tonexty',
  name: 'Setpoint',
  x: dates,
  y: targetTempVals,
  line: {
    color: '#9932CC', 
    shape: 'spline'
  }
}

var data = [curTempTrace,targetTempTrace];

var layout = {
  margin: {
    l: 20,
    r: 0,
    b: 20,
    t: 20,
    pad: 4
  }, 
  showlegend: true,
  legend:{
    xanchor:"center",
    yanchor:"top",
    y:-0.2,
    x:0.5
  },
  yaxis: {
    tickformat: '%H:%M:%S'
  }
};

var config = {
  responsive: true,
  displaylogo: false
}

Plotly.newPlot(chartDiv, data, layout, config);


function plotTemperature(jsonValue) {
  var keys = Object.keys(jsonValue);

  var date = new Date();
  console.log(date);
  dates.push(date);

  const curTempKey = keys[0];
  const targetTempKey = keys[1];

  var curTemp = Number(jsonValue[curTempKey]);
  console.log(curTemp);
  curTempVals.push(curTemp);

  var targetTemp = Number(jsonValue[targetTempKey]);
  console.log(targetTemp);
  targetTempVals.push(targetTemp);  
  
  Plotly.extendTraces(chartDiv, {
    x: [[dates[dates.length-1]], [dates[dates.length-1]]],
    y: [[curTempVals[curTempVals.length-1]], [targetTempVals[targetTempVals.length-1]]]
  }, [0, 1])

  if(dates.length > 600) {
      Plotly.relayout(chartDiv,
        {
          xaxis: {
              range: [dates[dates.length-600], dates[dates.length-1]]
          }
      });
  }
}


function getTemperatures(){
  var xhr = new XMLHttpRequest();

  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      plotTemperature(myObj);
    }
  };

  xhr.open("GET", "/temperatures", true);
  xhr.send();
}


if (!!window.EventSource) {
  var source = new EventSource('/events');

  source.addEventListener('open', function(e) {
    console.log("Event source connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events source disconnected");
    }
  }, false);

  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);

  source.addEventListener('new_temps', function(e) {
    console.log("new_temps", e.data);
    var myObj = JSON.parse(e.data);
    console.log(myObj);
    plotTemperature(myObj);
  }, false);
}
