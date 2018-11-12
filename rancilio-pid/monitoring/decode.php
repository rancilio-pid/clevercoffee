<?php

//include Connection
include './config/connection.php';
$mysqli = new mysqli(DB_HOST, DB_USERNAME, DB_PASSWORD, DB_NAME);
$nodes = array("", "", "");

//Nodes: Markus, Stefan, Konstantin


//Script läuft nur 1 Minute - danach exit (hintergrund 1und1 erlaubt nur scripte bis 1 Minute)
for ($sekunde = 0; $sekunde <12; $sekunde++) {

	//läuft alle Nodes durch
	for ($i = 0; $i <count($nodes); $i++) {
		
		$arrayNode = $nodes[$i];

		$json_isttemp = file_get_contents('http://blynk.remoteapp.de:8080/'.$arrayNode.'/get/V2');
		$data_isttemp = json_decode($json_isttemp,true);

		$json_solltemp = file_get_contents('http://blynk.remoteapp.de:8080/'.$arrayNode.'/get/V3');
		$data_solltemp = json_decode($json_solltemp,true);

		$json_pid_p = file_get_contents('http://blynk.remoteapp.de:8080/'.$arrayNode.'/get/V20');
		$data_pid_p = json_decode($json_pid_p,true);

		$json_pid_i = file_get_contents('http://blynk.remoteapp.de:8080/'.$arrayNode.'/get/V21');
		$data_pid_i = json_decode($json_pid_i,true);

		$json_pid_d = file_get_contents('http://blynk.remoteapp.de:8080/'.$arrayNode.'/get/V22');
		$data_pid_d = json_decode($json_pid_d,true);

		$json_output = file_get_contents('http://blynk.remoteapp.de:8080/'.$arrayNode.'/get/V23');
		$data_output = json_decode($json_output,true);

		$json_online = file_get_contents('http://blynk.remoteapp.de:8080/'.$arrayNode.'/isHardwareConnected');
		$data_online = json_decode($json_online,true);
	
		//Wenn der Node Online ist...
		if($data_online == 1 && $data_isttemp[0] > 30){
			echo "Status: ".$data_online;
			echo "<br>";
			echo round($data_isttemp[0],2);
			echo "/";
			echo round($data_solltemp[0],2);
			echo "<br>";
			echo $data_pid_p[0];
			echo "<br>";
			echo $data_pid_i[0];
			echo "<br>";
			echo $data_pid_d[0];
			echo "<br>";
			echo $data_output[0];
			echo "<br>";


			date_default_timezone_set("Europe/Berlin");
			$timestamp = time();
			$datum = date("Y-m-d",$timestamp);
			$uhrzeit = date("H:i:s",$timestamp);
			echo $datum." ".$uhrzeit," Uhr";
			echo "<br>";
					
			$isttemp = round($data_isttemp[0],2);
			$solltemp = round($data_solltemp[0],2);
			$pid_p = $data_pid_p[0];
			$pid_i = $data_pid_i[0];
			$pid_d = $data_pid_d[0];
			$pid_output = $data_output[0];
			$status = $data_online;
			$timestamp = $datum." ".$uhrzeit;
			
			$hostname =  $data['nodes'][$arrayNode]['nodeinfo']['hostname'];
			$hardware_model =   $data['nodes'][$arrayNode]['nodeinfo']['hardware']['model'];

			$sql = "INSERT INTO `nodes` (`id`, `node_id`, `isttemp`, `solltemp`, `status`, `pid_p`, `pid_i`, `pid_d`, `pid_output`, `timestamp`) VALUES (NULL, '$arrayNode', '$isttemp', '$solltemp', '$data_online', '$pid_p', '$pid_i', '$pid_d', '$pid_output', '$timestamp');";

			if ($mysqli->query($sql) === TRUE) {
				echo "New record created successfully<br>";
			} else {
				echo "Error: " . $sql . "<br>" . $mysqli->error;
			}
		}
	}
// 5 Sekunden warten
sleep(5);

}

//close connection
$mysqli->close();

//print_r($data);
exit;


?>