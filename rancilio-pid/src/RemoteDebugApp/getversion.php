<?php
///////////// PHP routines

// Headers

header('Content-Type: text/plain');
header('Access-Control-Allow-Origin: *');

// Utilities

include('php/version.php');
include('php/util.php');

util_inicializar("RemoteDebugApp");

function D($msg) {echo $msg . "<br>";}

// Codigo PHP para gravar o acesso e retornar a versao do app


// Localhost ?

$localhost = (startsWith(gethostname(), "MacBook-Pro"));

// Liga o debug

if ($localhost) {

    $debug = true;
    util_debug_ligar (true);
    util_debug_limpar();
}

// Get version

echo $appVersion;

// Save access, only on page load

if (isset($_GET["onload"])) {

    // Conecta ao banco de dados MySql/MariaDB

    if ($localhost) { // Local 

        // Conecta

        $conn_mysql = util_db_conectar("joaolopesf_net", "192.168.0.106",
                                        "root", "d@t@b@s3.");
                                    
    } else  { // Producao - GoDaddy - www.joaolopesf.net

        $conn_mysql = util_db_conectar("joaolopesf_net", "127.0.0.1",
                                        "joaolopesf", "d@t@b@s3.");
    }

    // Liga o debug

    if (isset($_GET["debug"]) == true) {
        $debug = true;
        util_debug_ligar (true);
        util_debug_limpar();
    }

    //util_grava_log ('I', "*** Inicio do processamento");

    // Data e hora

    $data = date('Y-m-d H:i:s');

    // Inicia a transacao

    util_db_begin_trans();

    $commit = true;

    // Adiciona acesso

    $sql = "INSERT INTO remotedebugapp_acesso
            (data, ip)
            VALUES (
                :data, :ip
            )";

    $sth = util_db_exec_sql($sql,   [   'data' => $data,
                                        'ip' => getRealIpAddr()
                                    ], true);

    if (is_null($sth)) {
        $causa = "insert remotedebugapp_acesso";
        util_grava_log('I',"util_db_exec_sql: retornou nulo - " . $causa);
        $commit = false;
    }

    // Finaliza a transacao

    if ($commit == true) {
        // Salva os dados
        util_db_commit_trans();
    } else {
        // Nao salva os dados
        util_db_rollback_trans();
    }
}

///// PHP - End
?>
