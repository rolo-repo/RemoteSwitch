

// HTML web page to handle 3 input fields (input1, input2, input3)
const char setupWiFiHTML[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
<title>ESP Input Form</title>
<meta name=viewport content="width=device-width, initial-scale=1">
<script>function myFunction(){var a=document.getElementById("password");if(a.type==="password"){a.type="text"}else{a.type="password"}};</script>
</head>
<body>
<div style=background-color:lightblue>
<form action=wifi>
<fieldset>
<legend>WiFi setting</legend>
<dl>
<dd><p style=text-align:left>SSID: <br/><input type=text size=30 name=ssid required/><br /></p></dd>
<dd><p style=text-align:left>Password:<br/> <input type=password size=30 id=password name="ssidp"/><input type=checkbox onclick=myFunction()>Reveal<br /></p></dd>
<dd><p style=text-align:left>BLYNK AUTH <br/><input type=text size=40 name=blynk_auth required/><br /></p></dd>
<dd><p style=text-align:left>BLYNK HOST <br/><input type=text size=30 name=blynk_domain value="blynk-cloud.com"/><br /></p></dd>
<dd><p style=text-align:left>BLYNK PORT <br/><input type=number size=10 name=blynk_port value="80"/><br /></p></dd>
<dd><p><input type=submit value=Save></p></dd>
</dl>
</fieldset>
</form>
</div>
</body>
</html>
)rawliteral";

const char success_html[] PROGMEM  = R"rawliteral(
<br>
<h1 style="text-align:center;">Thank You!</h1>
<hr>
<br><a href="/">Return to Home Page</a>
)rawliteral";


const char on_off_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<style>table{font - family:arial,sans - serif;border - collapse:collapse}td,th{border:1px solid #ddd;text - align:left;padding:8px}</style>
</head>
<body>
<input type=button onclick="location.href='/on'" value="ON"/>
<input type=button onclick="location.href='/off'" value="OFF"/>
</body>
</html>
)rawliteral";



