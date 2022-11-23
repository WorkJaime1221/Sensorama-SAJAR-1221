//Humedad temperatura trocado
//hacer ajuste de tiempo
//Dop = Exactitud de el GPS
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

//firebase
#include <Firebase_ESP_Client.h>
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
//#include <axp20x.h>

// NMTP server servidor de tiempo
String fechaHora = "";
byte last_second, Second, Minute, Hour, Day, Month;
int Year;


// gps
#define SERIAL1_RX 16 // GPS_TX -> 34
#define SERIAL1_TX 17 // 12 -> GPS_RX
String read_sentence;

TinyGPSPlus gps;
// The serial connection to the GPS device
SoftwareSerial ss(SERIAL1_RX, SERIAL1_TX);


//SoftwareSerial ss(12,34); // rx, tx
////HardwareSerial SerialGPS(1);

// Potencia
//AXP20X_Class axp;

// Serial
uint32_t chipId = 0;

// WIFI
const char* soft_ap_ssid = "SAJR-1221";
const char* soft_ap_password = "12345678";
const char* host = "SAJR-1221"; // corresponde al nombre del cansat


// Web Server
WebServer server(80);
WiFiMulti wifiMulti;

// Insert Firebase project API Key
#define API_KEY "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX-X" 

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://ulibreapp-default-rtdb.firebaseio.com" 

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
unsigned long sendDataRawPrevMillis = 0;
int count = 0;
int FBId = 0;
bool signupOK = false;


/*
 * Login page
 */

const char* loginIndex =
 "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
            "<td colspan=2>"
                "<center><font size=4><b>SAJR-1221 v1</b></font></center>"
                "<br>"
                "<center><font size=4><b>WiFi STA - AP</b></font>"
                "<span id='IP'>0</span></center>"
                "<br>"
                "<center><font size=4><b>GPS</b></font>"
                "<span id='GPS'>0</span></center>"
                "<br>"
                "<center><font size=4><b>ESP32 Login Page</b></font>"
            "</td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
             "<td>Username:</td>"
             "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
            "<td>Password:</td>"
            "<td><input type='Password' size=25 name='pwd'><br></td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
            "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
    "</table>"
"</form>"
"<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')"
    "}"
    "}"
    "function getIP() {"
    "  var xhttp = new XMLHttpRequest();"
    "  xhttp.onreadystatechange = function() {"
    "    if (this.readyState == 4 && this.status == 200) {"
    "      document.getElementById('IP').innerHTML = this.responseText;"
    "    }"
    "  };"
    "  xhttp.open('GET', 'readIP', true);"
    "  xhttp.send();"
    "}"
    "getIP();"
    "setInterval(function() {"
    "  getGPS();"
    "}, 3000);"
    "function getGPS() {"
    " if (this.responseText!=''){"
    "  var xhttp = new XMLHttpRequest();"
    "  xhttp.onreadystatechange = function() {"
    "    if (this.readyState == 4 && this.status == 200) {"
    "      document.getElementById('GPS').innerHTML = this.responseText;"
    "    }"
    "  };"
    "  xhttp.open('GET', 'readGPS', true);"
    "  xhttp.send();"
    " }"
    "}"
"</script>";

/*
 * Server Index Page
 */

const char* serverIndex =
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')"
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";

String IpAddress2String(const IPAddress& ipAddress)
{
    return String(ipAddress[0]) + String(".") +
           String(ipAddress[1]) + String(".") +
           String(ipAddress[2]) + String(".") +
           String(ipAddress[3]);
}

void handleIP() {
   String a = String(WiFi.SSID()) + "\n" + IpAddress2String(WiFi.localIP());
   String b = String(soft_ap_ssid) + "\n" + IpAddress2String(WiFi.softAPIP());
   //String adcValue = String(a);
   
   server.send(200, "text/plane", a+"\n"+b); //Send ADC value only to client ajax request
}

void handleGPS() {
   String a = GPSRead();
   
   server.send(200, "text/plane", a); //Send GPS value only to client ajax request
}

void FirebaseSetup() {
/* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  Serial.println("Ini Config FB SignUp.");
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok SignUp.");
    signupOK = true;
  }
  else{
    Serial.printf("No SignUp: %s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

/*
 * setup function
 */
void setup(void) {
  Serial.begin(115200);
  //Serial1.begin(9600, SERIAL_8N1, SERIAL1_RX, SERIAL1_TX); // GPS
  ss.begin(9600); // GPS

// serial
  for(int i=0; i<17; i=i+8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has %d cores\n", ESP.getChipCores());
  Serial.print("Chip ID: "); Serial.println(chipId);

// inicio wifi
  WiFi.mode(WIFI_MODE_APSTA);
  // 1. iniciamos modo AP
  // 1.1 configurar IP estática para el servidor
  WiFi.softAP(soft_ap_ssid, soft_ap_password);

  Serial.print("ESP32 IP as soft AP: ");
  Serial.println(WiFi.softAPIP());

// Add list of wifi networks
  wifiMulti.addAP("UNILIBRE", "");


  // buscar redes
  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
      Serial.println("no networks found");
  } 
  else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
      delay(10);
    }
  }

  // modo STA Multi
  if(wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
// fin wifi


  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started: http//" + String(host));
  
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });



  server.on("/readIP", handleIP);
  server.on("/readGPS", handleGPS);

  server.begin();

  FirebaseSetup();

}


// GPS
String GPSRead() {
  String mensaje = "LAT: " + String(gps.location.lat(),6) + " LON: " + String(gps.location.lng(),6) + " ALT: " + gps.altitude.meters() + " DOP: " + gps.hdop.value();;
  while (ss.available() > 0){
    gps.encode(ss.read());
    if (gps.location.isUpdated() || true){
      mensaje = "LAT: " + String(gps.location.lat(),6) + " LON: " + String(gps.location.lng(),6) + " ALT: " + gps.altitude.meters() + " DOP: " + gps.hdop.value();
    }
    // date time
    // date time
    fechaHora = GetDateTime(); // COnvertir Gmt a hora local
    
  }  
  return mensaje;
}

double GPSLatRead() { 
  return gps.location.lat();
}

double GPSLonRead() { 
  return gps.location.lng();
}

float GPSAlturaRead() { 
  return gps.altitude.meters();
}


double TemperaturaRead(){

  // leer del dht
  return 0.01 + random(15,45);
}

int HumedadRead(){
  // leer del dht
  return 40 + random(20,60);
}
float HallRead(){
  // leer del dht
  return 40.2 + random(300,900);
}
int UVRead(){
  // leer del dht
  return 1 + random(0,10);
}
float BMPAlturaRead(){
  // leer del BMP
  return 100.1 + random(1,6000);
}
float BMPPresionRead(){
  // leer del BMP
  return 1013.1 - random(1,500);  
}
float BMPTemperaturaRead(){
  // leer del BMP
  return 5.1 - random(5,50);    
}


String sentence_sep(String input, int index) {
  int finder = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = input.length() - 1;

  for (int i = 0; i <= maxIndex && finder <= index; i++) {
    if (input.charAt(i) == ',' || i == maxIndex) {  //',' = separator
      finder++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return finder > index ? input.substring(strIndex[0], strIndex[1]) : "";
}

float convert_gps_coord(float deg_min, String orientation) {
  double gps_min = fmod((double)deg_min, 100.0);
  int gps_deg = deg_min / 100;
  double dec_deg = gps_deg + ( gps_min / 60 );
  if (orientation == "W" || orientation == "S") {
    dec_deg = 0 - dec_deg;
  }
  return dec_deg;
}

void FirebaseLoop(){
  // Ir a la BD
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 3000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    String ruta = String(host)+ "-" + String(chipId) + "/default";
    // Write Humedad DHT
    if (Firebase.RTDB.setInt(&fbdo, ruta + "/Humedad", HumedadRead())){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    count++;
    
    // Write an Float number on the database path test/float DHT
    if (Firebase.RTDB.setDouble(&fbdo,  ruta + "/Temperatura", TemperaturaRead())){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    // Write GPS Data
    if (Firebase.RTDB.setString(&fbdo,  ruta + "/GPS", GPSRead())){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    // Write HAll Data
    if (Firebase.RTDB.setFloat(&fbdo,  ruta + "/Hall", HallRead())){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    // Write UV Data
    if (Firebase.RTDB.setInt(&fbdo,  ruta + "/UV", UVRead())){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }  
    
    // Write BMP Data Presión, Altura, Remperatura SetJSON
    if (Firebase.RTDB.setFloat(&fbdo,  ruta + "/BMPAltura", BMPAlturaRead())){ //BMPAlturaRead BMPPresionRead BMPTemperaturaRead
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    } 

        // Write BMP Data Presión, Altura, Remperatura SetJSON
    if (Firebase.RTDB.setFloat(&fbdo,  ruta + "/BMPPresion", BMPPresionRead())){ //BMPAlturaRead BMPPresionRead BMPTemperaturaRead
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    } 

        // Write BMP Data Presión, Altura, Remperatura SetJSON
    if (Firebase.RTDB.setFloat(&fbdo,  ruta + "/BMPTemperatura", BMPTemperaturaRead())){ //BMPAlturaRead BMPPresionRead BMPTemperaturaRead
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    } 
  }
  // inserta en RAW los datos para analisis la llave es String(host) + fecha + hora
  // los datos son GPSRead()+ DHTRead() + UVRead() + BMPRead() + HallRead()
  if (Firebase.ready() && signupOK && (millis() - sendDataRawPrevMillis > 9000 || sendDataRawPrevMillis == 0)){
    sendDataRawPrevMillis = millis();
    //String ruta = String(host) + String("/RAW/") + fechaHora + FBId
    String ruta = String(host)+ "-" + String(chipId) + String("/RAW/") + fechaHora + "-" + FBId;
    Serial.print( + " " + ruta);
    // Inserta un registro con todos los campos OJO la fecha y hora
    if (Firebase.RTDB.setDouble(&fbdo, ruta + "/Humedad", HumedadRead())){
      Serial.println("RAW PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("RAW FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    
    // Write an Float number on the database path test/float DHT
    if (Firebase.RTDB.setInt(&fbdo, ruta + "/Temperatura", TemperaturaRead())){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    // Write GPS Data
    if (Firebase.RTDB.setFloat(&fbdo, ruta + "/GPSAltura", GPSAlturaRead())){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.setDouble(&fbdo, ruta + "/GPSLat", GPSLatRead())){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.setDouble(&fbdo, ruta + "/GPSLon", GPSLonRead())){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    

    // Write HAll Data
    if (Firebase.RTDB.setFloat(&fbdo, ruta + "/Hall", HallRead())){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    // Write UV Data
    if (Firebase.RTDB.setInt(&fbdo, ruta + "/UV", UVRead())){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }  
    
    // Write BMP Data Presión, Altura, Remperatura SetJSON
    // Write BMP Data Presión, Altura, Remperatura SetJSON
    if (Firebase.RTDB.setFloat(&fbdo, ruta + "/BMPAltura", BMPAlturaRead())){ //BMPAlturaRead BMPPresionRead BMPTemperaturaRead
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    } 

        // Write BMP Data Presión, Altura, Remperatura SetJSON
    if (Firebase.RTDB.setFloat(&fbdo, ruta + "/BMPPresion", BMPPresionRead())){ //BMPAlturaRead BMPPresionRead BMPTemperaturaRead
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    } 

        // Write BMP Data Presión, Altura, Remperatura SetJSON
    if (Firebase.RTDB.setFloat(&fbdo, ruta + "/BMPTemperatura", BMPTemperaturaRead())){ //BMPAlturaRead BMPPresionRead BMPTemperaturaRead
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    
    FBId++;
  }
  
}

String GetDateTime(){
  char Horatxt[3];
  char Minutotxt[3];
  char Segundotxt[3];
  char Daytxt[3];
  char Mestxt[3];
// get time from GPS module
      if (gps.time.isValid())
      {
        //Minute = gps.time.minute();
        //Second = gps.time.second();
        //Hour   = gps.time.hour();
        sprintf(Horatxt, "%.2d", gps.time.hour());
        sprintf(Minutotxt, "%.2d", gps.time.minute());
        sprintf(Segundotxt, "%.4d", gps.time.second());
      }
 
      // get date drom GPS module
      if (gps.date.isValid())
      {
        //Day   = gps.date.day();
        sprintf(Daytxt, "%.2d", gps.date.day());
        //Month = gps.date.month();
        sprintf(Mestxt, "%.2d", gps.date.month());
        Year  = gps.date.year();
      } 
      return String(Year) + "-" + String(Mestxt) + "-" +String(Daytxt) + " " +String(Horatxt) + ":" +String(Minutotxt) + ":" +String(Segundotxt);
}

void loop(void) {
// web server  
  server.handleClient();

// tomar la fecha y hora actual
//  getLocalTime(&timeinfo);
//  char Dia[2];
//  char Mes[1];
//  char Year[4];
//  char Hora[2];
//  char Minutos[2];
//  char Segundos[2];
//  strftime(Dia,2, "%d", &timeinfo);
//  strftime(Mes,1, "%B", &timeinfo);
//  strftime(Year,4, "%Y", &timeinfo);
//  strftime(Hora,2, "%H", &timeinfo);
//  strftime(Minutos,2, "%M", &timeinfo);
//  strftime(Segundos,2, "%S", &timeinfo);
//  
//  fechaHora = String(Dia) + "-" + String(Mes) + "-" + String(Year) + " " + String(Hora) + "." + String(Minutos) + "." + String(Segundos);
  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

 
  delay(3000);
  
// GPS
  GPSRead();

// firebase
  FirebaseLoop();

  
  delay(1);
}
