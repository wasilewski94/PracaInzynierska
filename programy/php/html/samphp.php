<!DOCTYPE html>
<html>
<head>
</head>
<body>
    
<?php
// define variables and set to empty values
$probkowanie = $kanal = "";

if ($_SERVER["REQUEST_METHOD"] == "POST") {
  $probkowanie = test_input($_POST["probkowanie"]);
  $kanal= test_input($_POST["kanal"]);
}

function test_input($data) {
  $data = trim($data);
  $data = stripslashes($data);
  $data = htmlspecialchars($data);
  return $data;
}
?>
 
<h2>Pomiar z przetwornika MAX1202</h2>
<form method="post" action="<?php echo htmlspecialchars($_SERVER["PHP_SELF"]);?>">  
  Okres probkowania [s]: <input type="text" name="probkowanie">
  <br><br>
  Kanal przetwornika[1-8]: <input type="text" name="kanal">
  <br><br>
  <input type="submit" name="submit" value="Submit">
</form>

<?php
echo "<h2>Twoja konfiguracja:</h2>";
echo $probkowanie;
echo "<br>";
echo $kanal;
echo "<br>";
?>

</body>
</html>








