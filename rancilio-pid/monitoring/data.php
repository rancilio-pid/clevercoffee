<?php
//setting header to json
header('Content-Type: application/json');

include './config/connection.php';
$mysqli = new mysqli(DB_HOST, DB_USERNAME, DB_PASSWORD, DB_NAME);

if (isset($_GET["onlinestatus"]) && isset($_GET["node"])) {

    $node = $_GET["node"];

    switch ($node) {
        case "markus":
            $node = "";
            break;
        case "stefan":
        $node = "";
            break;
        case "konstantin":
        $node = "";
            break;
    }

    $json_online = file_get_contents('http://blynk.remoteapp.de:8080/'.$node.'/isHardwareConnected');
    echo $json_online;


}else{
    if (isset($_GET["lastemp"]) && isset($_GET["node"])) {

        $node = $_GET["node"];

        switch ($node) {
            case "markus":
                $node = "";
                break;
            case "stefan":
            $node = "";
                break;
            case "konstantin":
            $node = "";
                break;
        }

        $query = "SELECT * FROM `nodes` WHERE `node_id` LIKE '$node' ORDER BY `timestamp` DESC LIMIT 1";
        $result = $mysqli->query($query);
        //loop through the returned data
        $data = array();
        foreach ($result as $row) {
            $data[] = $row;
        }

        echo $data[0]['isttemp'];

        //free memory associated with result
        $result->close();

        //close connection
        $mysqli->close();


    }else{
        if (isset($_GET["node"])) {
            $node = $_GET["node"];

            switch ($node) {
                case "markus":
                    $node = "";
                    break;
                case "stefan":
                $node = "";
                    break;
                case "konstantin":
                $node = "";
                    break;
            }

            if (isset($_GET["history"])) {
                $history = $_GET["history"];
            }else{
                $history = '1440';
            }
            
            //query to get data from the table
            $query = sprintf("SELECT `isttemp`,`timestamp`,`solltemp`,`pid_p`,`pid_i`,`pid_d`,`pid_output` FROM `nodes` WHERE `node_id` LIKE '$node' AND `timestamp` > TIMESTAMP(DATE_SUB(NOW(), INTERVAL $history minute))");

            //execute query
            $result = $mysqli->query($query);

            //loop through the returned data
            $data = array();
            foreach ($result as $row) {
                $data[] = $row;
            }

            //now print the data
            print json_encode($data);

            //free memory associated with result
            $result->close();

            //close connection
            $mysqli->close();
        }else{
            exit;
        }
    }
}




if(!$mysqli){
	die("Connection failed: " . $mysqli->error);
}

