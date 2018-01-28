<!DOCTYPE html>
<html>
<head>
<h2>Pomiar cisnienia</h2>

<?php
    //plik konfiguracyjny
    $fconfig = fopen("konfiguracja.txt", "r"); 
    if($fconfig != false)
    {
        fscanf($fconfig, "%d", $okres);
        fclose($fconfig);
    }
    else{
        echo "Nie udalo sie otworzyc pliku konfiguracja.txt";   
    }
    //plik z pid dzialajacego procesu 
    $myfile = fopen("pid.txt", "r"); 
    if($myfile != false)
    {
        fscanf($myfile, "%d", $pid);
        fclose($myfile);
    }
    else{
        echo "Nie udalo sie otworzyc pliku pid.txt";   
    }

    if($_GET)
    {
        if(isset($_GET['start'])){
            start();
            $myfile = fopen("pid.txt", "w");
            if($myfile != false)
            {
                fwrite($myfile, $pid);
                fclose($myfile);     
            }
            else
            {
                echo "Nie udalo sie otworzyc pliku pid.txt";
            }
        }
                
        elseif(isset($_GET['stop']))
        {        
            stop();
            $myfile = fopen("pid.txt", "w");
            $pid = 999999;
            fwrite($myfile, $pid);
            fclose($myfile); 
        }
            
    }

function start()
{
    global $pid;
    if($pid != 999999)
    {
        echo "Proces " . "$pid" . " jest uruchomiony" ;
    }
    else
    {
        $pid = shell_exec("nohup /home/pi/inzynierka/LPS25H/LPS25H.e > /dev/null & echo $!");
        echo "Uruchomiono proces: " . "$pid";
    }
}

function stop()
{
    global $pid;
    if($pid == 999999)
    {
        echo "Proces zatrzymany";
    }
    else
    {
        exec("kill -KILL $pid");
        echo "Zatrzymano proces " . "$pid";
    } 
}
?>
    
</head>
<body>
  
  <p>Okres probkowania [s]: <?php echo $okres?></p>
  
<?php
// define variables and set to empty values
$probkowanie = $kanal = "";

if ($_SERVER["REQUEST_METHOD"] == "POST") {
  $probkowanie = test_input($_POST["probkowanie"]);

  $fconfigw = fopen("konfiguracja.txt", "w"); 
  if($fconfigw != false)
    {
        fwrite($fconfigw, "%d", $probkowanie);
        fclose($fconfigw);
    }
    else{
        echo "Nie udalo sie otworzyc pliku konfiguracja.txt";   
    }
    
  $kanal= test_input($_POST["kanal"]);
}

function test_input($data) {
  $data = trim($data);
  $data = stripslashes($data);
  $data = htmlspecialchars($data);
  
  return $data;
}
?>

<form method="post" action="<?php echo htmlspecialchars($_SERVER["PHP_SELF"]);?>">  
  Okres probkowania [s]: <input type="text" name="probkowanie">
  <br><br>
  Kanal przetwornika[1-8]: <input type="text" name="kanal">
  <br><br>
  <input type="submit" name="zapisz" value="Zapisz konfiguracje">
</form>


<form >
  <input type="submit" class="button" name="start" value="Start" />
  <input type="submit" class="button" name="stop" value="Stop" />
</form>

<?php
echo "<h2>Twoja konfiguracja:</h2>";
echo "Okres probkowania: " . "$probkowanie";
echo "<br>";
echo "Kanal przetwornika: " . "$kanal";
echo "<br>";
?>

</body>
</html> 

