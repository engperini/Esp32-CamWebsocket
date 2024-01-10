#include <Adafruit_BME280.h>
#include <ArduinoJson.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "Adafruit_VL53L1X.h"
#include <Wire.h>

TwoWire I2C = TwoWire(0);

#define I2C_SDA 15 //choose new pins 14/15
#define I2C_SCL 14

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C, OLED_RESET);
Adafruit_VL53L1X vl53 = Adafruit_VL53L1X();//(XSHUT_PIN, IRQ_PIN);
Adafruit_BME280 bme = Adafruit_BME280();

#define LED_BUILTIN 4 // GPIO do LED da câmera no ESP32-CAM

#include <SPIFFS.h>
#include <FS.h>
#define FILE_PHOTO "/photo.jpg"

//(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

//cria servidor websocket 
WebSocketsServer webSocket = WebSocketsServer(81);

//cria servidor HTTP
//WiFiServer server(80);
AsyncWebServer server(80);

uint8_t cam_num;
uint8_t sensor_num;
bool connected = false;

StaticJsonDocument<200> doc_rx; //receive from client
StaticJsonDocument<200> doc_tx; // not used yet, because the send is binary

String index_html = "<!DOCTYPE html>\n\
<html>\n\
<head>\n\
    <title>WebSockets Client</title>\n\
    <meta name='viewport' content='width=device-width, initial-scale=1'>\n\
    <link rel='stylesheet' href='https://use.fontawesome.com/releases/v5.7.2/css/all.css' integrity='sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr' crossorigin='anonymous'>\n\
    <link rel='icon' href='data:'>\n\
    <script src='http://code.jquery.com/jquery-1.9.1.min.js'></script>\n\
    <style>\n\
        body {\n\
            font-family: Arial, sans-serif;\n\
            text-align: center;\n\
            background-color: #f3f3f3;\n\
            margin: 0;\n\
            padding: 0;\n\
        }\n\
        #live {\n\
            max-width: 100%;\n\
            height: auto;\n\
            margin-top: 20px;\n\
            border: 2px solid #ccc;\n\
        }\n\
        .card-grid {\n\
            display: flex;\n\
            flex-wrap: wrap;\n\
            justify-content: center;\n\
        }\n\
        .bold { \n\
            font-weight: bold;\n\
        }\n\
        .card {\n\
            background-color: white;\n\
            box-shadow: 0 4px 8px 0 rgba(0, 0, 0, 0.2);\n\
            padding: 20px;\n\
            width: 200px;\n\
            margin: 10px; /* Ajuste a margem entre os cards */\n\
            text-align: left;\n\
        }\n\
        .reading {\n\
            font-size: 16px;\n\
        }\n\
        .buttons button {\n\
            padding: 15px 30px;\n\
            margin: 10px;\n\
            font-size: 16px;\n\
        }\n\
        .buttons {\n\
            display: flex;\n\
            justify-content: center;\n\
        }\n\
        .controls {\n\
            display: inline-block;\n\
            text-align: left;\n\
            width: 430px;\n\
            padding: 20px;\n\
        }\n\
        .controls .buttons {\n\
            display: flex;\n\
            flex-wrap: wrap;\n\
            justify-content: center;\n\
        }\n\
        .controls .buttons button {\n\
            padding: 15px 30px;\n\
            margin: 10px;\n\
            font-size: 16px;\n\
        }\n\
        .photo-container-wrapper {\n\
          text-align: center;\n\
          margin-top: 20px;\n\
        }\n\
        .photo-container-wrapper img {\n\
          max-width: 100%;\n\
          height: auto;\n\
          border: 2px solid #ccc;\n\
        }\n\
        .photo-container-wrapper button {\n\
          text-align: center; \n\
          display: block;\n\
          margin: 0 auto;\n\
          margin-top: 10px;\n\
          padding: 10px 20px;\n\
          font-size: 16px;\n\
        }\n\
    </style>\n\
</head>\n\
<body>\n\
    <h1>ESP32-CAM WebSocket</h1>\n\
    <div class='card-grid'>\n\
        <div class='card'>\n\
            <p class='card-title bold'><i class='fas fa-chart-line' style='color:#059e8a;'></i> Leitura de Sensores</p>\n\
            <p class='reading'><i class='fas fa-ruler' style='color:#059e8a;'></i> Distance: <span id='distanceValue'>--</span></p>\n\
            <p class='reading'><i class='fas fa-tachometer-alt' style='color:#059e8a;'></i> Pressure: <span id='pressureValue'>--</span></p>\n\
            <p class='reading'><i class='fas fa-mountain' style='color:#059e8a;'></i> Altitude: <span id='altitudeValue'>--</span></p>\n\
            <p class='reading'><i class='fas fa-thermometer-half' style='color:#059e8a;'></i> Temperature: <span id='temperatureValue'>--</span></p>\n\
            <p class='reading'><i class='fas fa-tint' style='color:#059e8a;'></i> Humidity: <span id='humidityValue'>--</span></p>\n\
        </div>\n\
    </div>\n\
    <div class='card controls'>\n\
        <p class='card-title bold'><i class='fas fa-cogs' style='color:#059e8a;'></i> Controls</p>\n\
        <div class='buttons'>\n\
            <button id='buttonLight'>Light</button>\n\
            <button id='buttonPhoto'>Photo</button>\n\
            <button id='buttonReset'>Reset</button>\n\
            <label for='autoPhoto'>Automatic Photo</label>\n\
            <input type='radio' id='autoPhoto' name='photoMode' value='auto'>\n\
        </div>\n\
    </div>\n\
    <h2>Live Stream</h2>\n\
    <img id='live' src=''>\n\
    <h2>Photo View</h2>\n\
    <div class='photo-container-wrapper'>\n\
      <img id='photo-container' src=''>\n\
      <button id='download-btn' style='display: block; margin-top: 10px;'>Download Photo</button>\n\
    </div>\n\    
    <script>\n\
        jQuery(function($) {\n\
            if (!('WebSocket' in window)) {\n\
                alert('Your browser does not support web sockets');\n\
            } else {\n\
                setup();\n\
            }\n\
            function setup() {\n\
                var host = 'ws://server_ip';\n\
                var host_rem = 'ws://server_ip_rem';\n\
                var socket = new WebSocket(host);\n\
                socket.binaryType = 'arraybuffer';\n\
                if (socket) {\n\
                    socket.onopen = function() {\n\
                        // Add any specific actions on socket open\n\
                    }\n\
                    socket.onmessage = function(msg) {\n\
                        var bytes = new Uint8Array(msg.data);\n\
                        var binary = '';\n\
                        var len = bytes.byteLength;\n\
                        if (bytes[0] === 0xAA) {\n\
                            //console.log(bytes);\n\
                            //console.log(len);\n\
                            var sensorData = new Float32Array(msg.data.slice(1, 21));\n\
                            //console.log(sensorData);\n\
                            console.log('Distance: ' + sensorData[0]);\n\
                            console.log('Temperature: ' + sensorData[1]);\n\
                            console.log('Pressure: ' + sensorData[2]);\n\
                            console.log('Altitude: ' + sensorData[3]);\n\
                            console.log('Humidity: ' + sensorData[4]);\n\
                            var distanceFormatted = sensorData[0] + ' mm';\n\
                            var temperatureFormatted = sensorData[1].toFixed(2) + ' C';\n\
                            var pressureFormatted = sensorData[2].toFixed(2) + ' Pa';\n\
                            var altitudeFormatted = sensorData[3].toFixed(2) + ' m';\n\
                            var humidityFormatted = sensorData[4].toFixed(2) + ' %';\n\
                            document.getElementById('distanceValue').innerText = distanceFormatted;\n\
                            document.getElementById('temperatureValue').innerText = temperatureFormatted;\n\
                            document.getElementById('pressureValue').innerText = pressureFormatted;\n\
                            document.getElementById('altitudeValue').innerText = altitudeFormatted;\n\
                            document.getElementById('humidityValue').innerText = humidityFormatted;\n\
                         } else if (bytes[0] === 0x01) {\n\
                            var commandfeedData = msg.data.slice(1);\n\
                            console.log(commandfeedData)\n\
                            console.log('Feedback Lampada: ' + commandfeedData[0])\n\
                            console.log('Feedback Photo: ' + commandfeedData[1])\n\
                            console.log('Feedback Reset: ' + commandfeedData[2])\n\
                         } else if (bytes[0] === 0x02) {\n\
                            console.log('Recebendo foto');\n\
                            // Remontar a imagem a partir dos dados recebidos\n\
                            var binary = '';\n\
                            for (var i = 1; i < len; i++) {\n\
                                binary += String.fromCharCode(bytes[i]);\n\
                            }\n\
                            var photoContainer = document.getElementById('photo-container');\n\
                            photoContainer.src = 'data:image/jpg;base64,' + window.btoa(binary);\n\
                            // Adicionando o evento de clique do botão de download\n\
                            var downloadBtn = document.getElementById('download-btn');\n\
                            downloadBtn.addEventListener('click', function() {\n\
                              var photoContainer = document.getElementById('photo-container');\n\
                              var imageSrc = photoContainer.src;\n\
                              var link = document.createElement('a');\n\
                              link.href = imageSrc;\n\
                              link.download = 'photo.jpg';\n\
                              link.click();\n\
                          });\n\                            
                         } else {\n\
                            for (var i = 0; i < len; i++) {\n\
                                binary += String.fromCharCode(bytes[i])\n\
                            }\n\
                            var img = document.getElementById('live');\n\
                            img.src = 'data:image/jpg;base64,' + window.btoa(binary);\n\
                        }\n\
                    }\n\
                    socket.onclose = function() {\n\
                        showServerResponse('The connection has been closed.');\n\
                    }\n\
                }\n\
                $('.buttons button').click(function() {\n\
                  var buttonName = this.id;\n\
                  var buttonData = { button_name: buttonName };\n\
                  console.log('Button clicked: ' + buttonName);\n\
                  socket.send(JSON.stringify(buttonData));\n\
                });\n\
                var isChecked = 0;\n\
                $('#autoPhoto').on('click', function() {\n\
                  if (isChecked){\n\
                    console.log('ligado');\n\
                    $(this).prop('checked', false);\n\
                    isChecked = 0;\n\
                  } else {\n\
                    console.log('desligado');\n\
                    $(this).prop('checked', true);\n\
                    isChecked = 1;\n\
                  }\n\
                  var autoPhotoState = $(this).is(':checked');\n\
                  var buttonName = 'autoPhoto_' + (autoPhotoState ? 'on' : 'off');\n\
                  var buttonData = { button_name: buttonName };\n\
                  console.log('Auto Photo state changed: ' + autoPhotoState);\n\
                  console.log(buttonData);\n\
                  socket.send(JSON.stringify(buttonData));\n\
              });\n\
            }\n\
        });\n\
    </script>\n\
</body>\n\
</html>\n";




void configCamera(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

//cam streaming websocket-------------------------------------------------------------------

void liveCam(uint8_t num){
  //capture a frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
      Serial.println("Frame buffer could not be acquired");
      return;
  }
  //send frames
  webSocket.sendBIN(num, fb->buf, fb->len);

  //return the frame buffer back to be reused
  esp_camera_fb_return(fb);
}

//websocket event--------------------------------------------------------------------------
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            cam_num = num;
            connected = true;
            Serial.printf("[%u] Client Connected!\n", num);
            break;
        case WStype_TEXT: // check client response
            DeserializationError error_json = deserializeJson(doc_rx, payload);
            if(error_json){
                Serial.print("jason failed");
                return;
            } 
            else {
                const char* b_name = doc_rx["button_name"];
                Serial.println("Received MSG: " + String(b_name));
                handleWebSocketMessage(num, String(b_name));
             }
            break;
//        case WStype_BIN:
//        case WStype_ERROR:     
//        case WStype_FRAGMENT_TEXT_START:
//        case WStype_FRAGMENT_BIN_START:
//        case WStype_FRAGMENT: 
//        case WStype_FRAGMENT_FIN:
//          break;
           
    }
}


String ServerIP;
void setup() {


  I2C.begin(I2C_SDA, I2C_SCL, 100000); // use with new pins
  
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  //---setup wifi---------------------------------------------------------------------------------
  Serial.begin(115200);
  WiFi.begin("XXXXXXXX", "XXXXXXXX");
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //------server and protocol setup---------------------------------------------------------------
  
  Serial.println("");
  ServerIP = WiFi.localIP().toString();
  Serial.print("Server Started IP address: " + ServerIP);

  //String LocalServerIPSocket = ServerIP +":81";
  //String RemoteServerIPSocket = "engperini.ddns.net:32521";
  
  //index_html.replace("server_ip", LocalServerIPSocket); // Local or RemoteServerIPSocket

  server.on("/", HTTP_GET, handleRequest);

  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  configCamera();
  
  //--setup distance v153x------------------------------------------------------------------------
  

  if (! vl53.begin(0x29, &I2C)) {
    Serial.print(F("Error on init of VL sensor: "));
    Serial.println(vl53.vl_status);
    while (1)       delay(10);
  }
  
  Serial.println(F("VL53L1X sensor OK!"));
  Serial.print(F("Sensor ID: 0x"));
  Serial.println(vl53.sensorID(), HEX);

  if (! vl53.startRanging()) {
    Serial.print(F("Couldn't start ranging: "));
    Serial.println(vl53.vl_status);
    while (1)       delay(10);
  }
 
  // Valid timing budgets: 15, 20, 33, 50, 100, 200 and 500ms!
  vl53.setTimingBudget(50);
  //Serial.print(F("Timing budget (ms): "));
  //Serial.println(vl53.getTimingBudget());

  /*
  vl.VL53L1X_SetDistanceThreshold(100, 300, 3, 1);
  vl.VL53L1X_SetInterruptPolarity(0);
  */


  //---setup bmp280-------------------------------------------------------------------------------

  unsigned status;
  status = bme.begin(0x76, &I2C);
  

  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
  Serial.println(F("Sensor BME280 ready"));

  //------------displaySetup-------------------------------------------------------------

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

    // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(1000); // Pause for 1 second
   
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.setCursor(15, 10);
  display.print("Sophia");
  display.display();
  delay(1000);

  //filesystem mount
  if (!SPIFFS.begin(true)) {
  Serial.println("An Error has occurred while mounting SPIFFS");
  ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }
  
}


void handleRequest(AsyncWebServerRequest *request) {
  
  String LocalServerIPSocket = ServerIP +":81";
  String RemoteServerIPSocket = "engperini.ddns.net:32522";
  
  String clientIP = request->client()->remoteIP().toString();
  Serial.println(clientIP);
  
  String localIPRange = "192.168."; // Defina aqui a faixa de IPs locais
  
  if (clientIP.startsWith(localIPRange)) {
    Serial.println("Client is local.");
    Serial.println(clientIP);
    index_html.replace("server_ip", LocalServerIPSocket);
  } else {
    Serial.println("Client is remote.");
    Serial.println(clientIP);
    index_html.replace("server_ip", RemoteServerIPSocket);
  }

  request->send(200, "text/html", index_html);
}

// Control-------------------------------------------------------------------------------

bool statusLight = LOW;
void handleWebSocketMessage(uint8_t num, String msg) {
  
  if (msg == "buttonLight") {
    bool currentLightStatus = digitalRead(LED_BUILTIN);
    
    if (currentLightStatus == LOW) {
        digitalWrite(LED_BUILTIN, HIGH); // Ligar o LED 
        statusLight = HIGH;
      }
    else {
      digitalWrite(LED_BUILTIN, LOW); // Ligar o LED 
      statusLight = LOW;
    }
  }

  else if (msg == "buttonPhoto") {

    capturePhotoSaveSpiffs();
    sendPhotoData();
  }

  else if (msg == "autoPhoto_on") {

    Serial.println("Server received automatic photo ON command from client");
    
  }

  else if (msg == "autoPhoto_off") {

    Serial.println("Server received automatic photo OFF command from client");
  }

  else if (msg == "buttonReset") {

    Serial.println("Server received RESET command from client");
    ESP.restart();
  }

  
  getcommandData();

 
}



// Sensors
float distance;
float temperatureValue;
float pressureValue; 
float altitudeValue; 
float humidityValue;

// Cria um array de bytes para armazenar os dados dos sensores
byte sensorData[21]; // Tamanho ajustado conforme a necessidade dos dados



//iniciando contadores
unsigned long previousMillis = 0; 
const long interval = 1000; //tempo de aquisição dos sensores

unsigned long previousMillis1 = 0;
unsigned long previousMillis3 = 0;
const long interval1 = 3000;//tempo de atualizacao websocket sensores

bool lamp_ok = false;
bool photo_ok = false;
bool reset_ok = false;

//Cria um array de bytes para o feedback dos comandos para o cliente
byte commandData[4]; // Inicializa com o cabeçalho



void getcommandData(){

  commandData[0] = 0x01;
  memcpy(&commandData[1], &commandData, digitalRead(LED_BUILTIN)); 
  memcpy(&commandData[2], &commandData, photo_ok); 
  memcpy(&commandData[3], &commandData, reset_ok); 

  webSocket.sendBIN(cam_num, commandData, sizeof(commandData)); 
}

void sendPhotoData() {
  // Check if the file exists and try to open it for reading
  File file = SPIFFS.open(FILE_PHOTO, FILE_READ);
  
  if (!file || file.isDirectory()) {
    Serial.println("Failed to open file for reading");
    return;
  }
  
  // Allocate a buffer to hold the file contents
  size_t fileSize = file.size();
  size_t totalSize = fileSize + 1; // Add 1 byte for the header
  uint8_t *buffer = (uint8_t *)malloc(totalSize);
  
  if (!buffer) {
    Serial.println("Failed to allocate memory for reading file");
    file.close();
    return;
  }
  
  // Set the header as the first byte
  buffer[0] = 0x02; // Header value (adjust as needed)
  
  // Read the file into the buffer, starting from the second byte
  size_t bytesRead = file.read(buffer + 1, fileSize);
  file.close();
  
  if (bytesRead != fileSize) {
    Serial.println("Error reading file");
    free(buffer);
    return;
  }
  
  // Send the file data (with header) via WebSocket
  webSocket.sendBIN(cam_num, buffer, totalSize);
  
  // Free the allocated buffer
  free(buffer);
}



void getsensorData() {

  unsigned long currentMillis = millis();
  // Verifica se passou o intervalo desejado
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    if (vl53.dataReady()) {
      // new measurement for the taking!
      distance = vl53.distance();
      if (distance == -1) {
        // something went wrong!
        Serial.print(F("Couldn't get distance: "));
        Serial.println(vl53.vl_status);
        //return;
      }
      Serial.print(F("Distance: "));
      Serial.print(distance);
      Serial.println(" mm");
  
      // data is read out, time for another reading!
      vl53.clearInterrupt();
    }

    temperatureValue = bme.readTemperature();
    pressureValue = bme.readPressure();
    altitudeValue = bme.readAltitude(1011.9); /* Adjusted to local forecast! */
    humidityValue = bme.readHumidity();

    Serial.print(F("Temperature = "));
    Serial.print(temperatureValue);
    Serial.println(" *C");
    
    Serial.print(F("Pressure = "));
    Serial.print(pressureValue);
    Serial.println(" Pa");

    Serial.print(F("Approx altitude = "));
    Serial.print(altitudeValue);
    Serial.println(" m");
    Serial.println();

    Serial.print(F("Humidity = "));
    Serial.print(humidityValue);
    Serial.println(" %");
    Serial.println();
   
  }
}


int page = 0;
void showDisplay() {

  unsigned long currentMillis3 = millis();
  // Verifica se passou o intervalo desejado
  if (currentMillis3 - previousMillis3 >= interval1) { //same interval of websocket 
    previousMillis3 = currentMillis3;

    page = page + 1;
    display.clearDisplay();
    
    if (1 == page) {

      display.setTextSize(1);
      display.setCursor(0,0);
      display.print("Temperature: ");
      display.setTextSize(2);
      display.setCursor(0,10);
      display.print((float) temperatureValue);
      display.print(" ");
      display.setTextSize(1);
      display.cp437(true);
      display.write(167);
      display.setTextSize(2);
      display.print("C");
      
    } else if (2 == page) {
    
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("Distance: ");
      display.setTextSize(2);
      display.setCursor(0, 10);
      display.print((int) distance);
      display.print(" mm");
        
    } else if (3 == page) {
    
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("Pressure: ");
      display.setTextSize(2);
      display.setCursor(0, 10);
      display.print((int) pressureValue);
      display.print(" Pa");

    } else if (4 == page) {
    
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("Altitude: ");
      display.setTextSize(2);
      display.setCursor(0, 10);
      display.print((int) altitudeValue);
      display.print(" m");

    } else if (5 == page) {
    
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("Humidity: ");
      display.setTextSize(2);
      display.setCursor(0, 10);
      display.print((int) humidityValue);
      display.print(" %");

      page = 0;

    }
    display.display();
  }
}

void sendSensorData() {

  unsigned long currentMillis1 = millis();
  // Verifica se passou o intervalo desejado
  if (currentMillis1 - previousMillis1 >= interval1) {
    previousMillis1 = currentMillis1;
    
    // Adiciona um cabeçalho (exemplo: 0xAA) para identificar os dados dos sensores
    sensorData[0] = 0xAA;
    
    // Converte os valores dos sensores para bytes e adiciona ao array
    memcpy(&sensorData[1], &distance, sizeof(distance));
    memcpy(&sensorData[5], &temperatureValue, sizeof(temperatureValue));
    memcpy(&sensorData[9], &pressureValue, sizeof(pressureValue));
    memcpy(&sensorData[13],&altitudeValue, sizeof(altitudeValue));
    memcpy(&sensorData[17],&humidityValue, sizeof(humidityValue));
    //sensorData[20] = 0x00;
    
    // Envie os dados dos sensores via WebSocket quando disponível
    if (connected) {
      
      webSocket.sendBIN(cam_num, sensorData, sizeof(sensorData)); // Envie os dados do sensor como bytes via WebSocket
      
    }
  }
}

boolean takeNewPhoto = false;

bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( FILE_PHOTO );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}

void capturePhotoSaveSpiffs( void ) {
  camera_fb_t * fb = NULL; // pointer
  bool ok = 0; // Boolean indicating if the picture has been taken correctly

  do {
    // Take a photo with the camera
    Serial.println("Taking a photo...");

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    // Photo file name
    Serial.printf("Picture file name: %s\n", FILE_PHOTO);
    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);

    // Insert the data in the photo file
    if (!file) {
      Serial.println("Failed to open file in writing mode");
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.print("The picture has been saved in ");
      Serial.print(FILE_PHOTO);
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes");

      
    }
    // Close the file
    file.close();
    esp_camera_fb_return(fb);

    // check if file has been correctly saved in SPIFFS
    ok = checkPhoto(SPIFFS);
  } while ( !ok );
}

void loop() {
  getsensorData();
  showDisplay();
  //http_resp();
  webSocket.loop();
  if(connected == true){
    liveCam(cam_num);
    sendSensorData();
  }
}
