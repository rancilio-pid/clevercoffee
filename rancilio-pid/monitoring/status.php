<!DOCTYPE html>
<?php
    if (isset($_GET["history"])) {
        $history = $_GET["history"];
    }else{
        $history = '1440';
    }
    if (isset($_GET["node"])) {
        $node = $_GET["node"];
    }else{
        $node = "markus";
    }
?>
<html>
	<head>
		<title>Rancilio-PID Anzeige</title>
        <meta http-equiv="refresh" content="60; url="<?php echo $_SERVER['PHP_SELF']; ?>""/>
		<style type="text/css">
			#image-container {
				width: 368px;
				height: 613px;
                float:left;
                background-image:url(rancilio-silvia.jpg);
                background-repeat: no-repeat;
			}
			#chart-container {
				width: 900px;
				height: auto;
                float:left;
			}
            #chart-navigation ul{
                list-style: none;
                text-align: center;
               
			}
            #chart-navigation li{
                list-style: none;
                display: inline-block;
                color:black;
			}
            #chart-navigation a{
                text-decoration: none;
                color:black;
            }
            #chart-navigation a:focus,
            #chart-navigation a:hover,
            #chart-navigation a:active {    
             text-decoration:underline;
             color:black;
            }
            #imagetemp{
                position:relative;
                top:220px;
                left:150px;
                font-size:20pt;
            }
            #imagestatus{
                position:relative;
                top:110px;
                left:191px;
                font-size:20pt;
            }
            #imagestatus div{
                position:relative;
                background-color:#10201c;
                width:15px;
                height:37px;
            }
		</style>
	</head>
	<body>
        <div id="image-container">
            <div id="imagetemp">
            <?php 
            $json_isttemp = file_get_contents('http://monitoring.rancilio-pid.de/data.php?lastemp=1&node='.$node.'');
            echo $json_isttemp."°C";
            ?>
            </div>
            <div id="imagestatus">
            <?php 
            $onlinestatus = file_get_contents('http://monitoring.rancilio-pid.de/data.php?onlinestatus=1&node='.$node.'');
            if($onlinestatus == 'false'){
                echo "<div>&nbsp;</div>";
            }
            ?>
            </div>
        </div>
		<div id="chart-container">
            <canvas id="mycanvas"></canvas>
            <ul id="chart-navigation">
                <a href="?node=<?php echo $node;?>&history=10"><li>10 minuten</li></a>
                <a href="?node=<?php echo $node;?>&history=30"><li>30 minuten</li></a>
                <a href="?node=<?php echo $node;?>&history=60"><li>1 Stunde</li></a>
                <a href="?node=<?php echo $node;?>&history=120"><li>2 Stunden</li></a>
                <a href="?node=<?php echo $node;?>&history=240"><li>4 Stunden</li></a>
                <a href="?node=<?php echo $node;?>&history=360"><li>6 Stunden</li></a>
                <a href="?node=<?php echo $node;?>&history=720"><li>12 Stunden</li></a>
                <a href="?node=<?php echo $node;?>&history=1440"><li>24 Stunden</li></a>
                <a href="?node=<?php echo $node;?>&history=10080"><li>1 Woche</li></a>
            </ul>
        </div>

		<!-- javascript -->
		<script type="text/javascript" src="https://cdnjs.cloudflare.com/ajax/libs/jquery/3.3.1/jquery.min.js"></script>
		<script type="text/javascript" src="js/Chart.min.js"></script>
        <script type="text/javascript">
            $(document).ready(function(){
                $.ajax({
                     url: "http://monitoring.rancilio-pid.de/data.php?node=<?php echo $node; ?>&history=<?php echo $history; ?>",
                    method: "GET",
                    success: function(data) {
                        var timestamp = [];
                        var isttemp = [];
                        var solltemp = [];
						var pid_p = [];
						var pid_i = [];
						var pid_d = [];
						var pid_output = [];
                        
                        for(var i in data) {
                            timestamp.push(data[i].timestamp);
                            isttemp.push(data[i].isttemp);
                            solltemp.push(data[i].solltemp);
							pid_p.push(data[i].pid_p);
							pid_i.push(data[i].pid_i);
							pid_d.push(data[i].pid_d);
							pid_output.push(data[i].pid_output);
                        }
                        var chartdata = {
                            labels: timestamp,
                            datasets : [
                                {
                                    label: 'IST',
                                    backgroundColor: 'rgba(46, 138, 138, 1)',
                                    borderColor: 'rgba(46, 138, 138, 1)',
                                    fill: false,
                                    data: isttemp,
                                    type: 'line',
                                    pointRadius: 0,
                                    fill: false,
                                    lineTension: 0,
                                    borderWidth: 2
                                },
                                {
                                    label: 'SOLL',
                                    backgroundColor: 'rgba(220, 71, 71, 1)',
                                    borderColor: 'rgba(220, 71, 71, 1)',
                                    fill: false,
                                    data: solltemp,
                                    type: 'line',
                                    pointRadius: 0,
                                    fill: false,
                                    lineTension: 0,
                                    borderWidth: 2
                                },
                                {
                                    label: 'pid_p',
                                    backgroundColor: 'rgba(255,100,20,0.8)',
                                    borderColor: 'rgba(255,100,20,0.8)',
                                    fill: false,
                                    data: pid_p,
                                    type: 'line',
                                    pointRadius: 0,
                                    fill: false,
                                    lineTension: 0,
									hidden: true,
                                    borderWidth: 2
                                },	
                                {
                                    label: 'pid_i',
                                    backgroundColor: 'rgba(255,100,20,0.5)',
                                    borderColor: 'rgba(255,100,20,0.5)',
                                    fill: false,
                                    data: pid_i,
                                    type: 'line',
                                    pointRadius: 0,
                                    fill: false,
                                    lineTension: 0,
									hidden: true,
                                    borderWidth: 2
                                },
                                {
                                    label: 'pid_d',
                                    backgroundColor: 'rgba(255,100,20,0.2)',
                                    borderColor: 'rgba(255,100,20,0.2)',
                                    fill: false,
                                    data: pid_d,
                                    type: 'line',
                                    pointRadius: 0,
                                    fill: false,
                                    lineTension: 0,
									hidden: true,
                                    borderWidth: 2
                                },	
                                {
                                    label: 'pid_output',
                                    backgroundColor: 'rgba(128,0,128, 1)',
                                    borderColor: 'rgba(128,0,128, 1)',
                                    fill: false,
                                    data: pid_output,
                                    type: 'line',
                                    pointRadius: 0,
                                    fill: false,
                                    lineTension: 0,
									hidden: true,
                                    borderWidth: 2
                                }
                            ]
                        };

                        var ctx = $("#mycanvas");

                        var barGraph = new Chart(ctx, {
                            type: 'line',
                            data: chartdata,
                            options: {
                                animation: {
                                duration: 0
                                }
                            }
                        });
                    },
                    error: function(data) {
                        console.log(data);
                    }
                });
            });        
        
        </script>
	</body>
</html>