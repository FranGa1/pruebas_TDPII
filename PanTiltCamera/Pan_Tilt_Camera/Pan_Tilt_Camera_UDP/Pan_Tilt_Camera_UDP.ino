#include "esp_camera.h"
#include "sensor.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <iostream>
#include <sstream>
#include <ESP32Servo.h>
#include "camera_pins.h"

#define DUMMY_SERVO1_PIN 12     //We need to create 2 dummy servos.
#define DUMMY_SERVO2_PIN 13     //So that ESP32Servo library does not interfere with pwm channel and timer used by esp32 camera.

#define PAN_PIN 14
#define TILT_PIN 15

Servo dummyServo1;
Servo dummyServo2;
Servo panServo;
Servo tiltServo;

// const char* ssid     = "NowISeeYou";
// const char* password = "12345678";

const char* ssid     = "FBI WiFi";
const char* password = "familylam180";

AsyncWebServer server(80);
AsyncWebSocket wsCamera("/Camera");
AsyncWebSocket wsServoInput("/ServoInput");
uint32_t cameraClientId = 0;

#define LIGHT_PIN 4
const int PWMLightChannel = 4;

const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>
    .noselect {
      -webkit-touch-callout: none; /* iOS Safari */
        -webkit-user-select: none; /* Safari */
         -khtml-user-select: none; /* Konqueror HTML */
           -moz-user-select: none; /* Firefox */
            -ms-user-select: none; /* Internet Explorer/Edge */
                user-select: none; /* Non-prefixed version, currently
                                      supported by Chrome and Opera */
    }

    .slidecontainer {
      width: 100%;
    }

    .slider {
      -webkit-appearance: none;
      width: 100%;
      height: 20px;
      border-radius: 5px;
      background: #d3d3d3;
      outline: none;
      opacity: 0.7;
      -webkit-transition: .2s;
      transition: opacity .2s;
    }

    .slider:hover {
      opacity: 1;
    }
  
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 40px;
      height: 40px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }

    .slider::-moz-range-thumb {
      width: 40px;
      height: 40px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }

    </style>
  
  </head>
  <body class="noselect" align="center" style="background-color:white">
     
    <!--h2 style="color: teal;text-align:center;">Wi-Fi Camera &#128663; Control</h2-->
    
    <table id="mainTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
      <tr>
        <img id="cameraImage" src="" style="width:400px;height:250px"></td>
      </tr> 
      <tr/><tr/>
      <tr>
        <td style="text-align:left"><b>Pan:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="130" value="0" class="slider" id="Pan" oninput='sendButtonInput("Pan",value)'>
          </div>
        </td>
      </tr> 
      <tr/><tr/>       
      <tr>
        <td style="text-align:left"><b>Tilt:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Tilt" oninput='sendButtonInput("Tilt",value)'>
          </div>
        </td>   
      </tr>
      <tr/><tr/>       
      <tr>
        <td style="text-align:left"><b>Light:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="255" value="0" class="slider" id="Light" oninput='sendButtonInput("Light",value)'>
          </div>
        </td>   
      </tr>      
    </table>
    <tr>
      <td colspan="3" style="text-align:center; padding-top:20px;">
          <button onclick="resetServos()" style="background-color: red; color: white; padding: 20px 40px; font-size: 20px;">Reset</button>
      </td>
    </tr>
    <tr>
      <td colspan="3" style="text-align:center; padding-top:20px;">
          <button onclick="demoServos()" style="background-color: green; color: white; padding: 20px 40px; font-size: 20px;">Demo</button>
      </td>
    </tr>


  
    <script>
      var webSocketCameraUrl = "ws:\/\/" + window.location.hostname + "/Camera";
      var webSocketServoInputUrl = "ws:\/\/" + window.location.hostname + "/ServoInput";      
      var websocketCamera;
      var websocketServoInput;

      function resetServos() {
        document.getElementById("Pan").value = 0;
        document.getElementById("Tilt").value = 90;
        websocketServoInput.send("ResetServos");
      }

      function demoServos() {
        websocketServoInput.send("DemoServos");
      }

      
      function initCameraWebSocket() 
      {
        websocketCamera = new WebSocket(webSocketCameraUrl);
        websocketCamera.binaryType = 'blob';
        websocketCamera.onopen    = function(event){};
        websocketCamera.onclose   = function(event){setTimeout(initCameraWebSocket, 2000);};
        websocketCamera.onmessage = function(event)
        {
          var imageId = document.getElementById("cameraImage");
          imageId.src = URL.createObjectURL(event.data);
        };
      }
      
      function initServoInputWebSocket() 
      {
        websocketServoInput = new WebSocket(webSocketServoInputUrl);
        websocketServoInput.onopen    = function(event)
        {
          var panButton = document.getElementById("Pan");
          sendButtonInput("Pan", panButton.value);
          var tiltButton = document.getElementById("Tilt");
          sendButtonInput("Tilt", tiltButton.value);
          var lightButton = document.getElementById("Light");
          sendButtonInput("Light", lightButton.value);          
        };
        websocketServoInput.onclose   = function(event){setTimeout(initServoInputWebSocket, 2000);};
        websocketServoInput.onmessage = function(event){};        
      }
      
      function initWebSocket() 
      {
        initCameraWebSocket ();
        initServoInputWebSocket();
      }

      function sendButtonInput(key, value) 
      {
        var data = key + "," + value;
        websocketServoInput.send(data);
      }
    
      window.onload = initWebSocket;
      document.getElementById("mainTable").addEventListener("touchend", function(event){
        event.preventDefault()
      });      
    </script>
  </body>    
</html>
)HTMLHOMEPAGE";

void handleRoot(AsyncWebServerRequest *request) 
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) 
{
    request->send(404, "text/plain", "File Not Found");
}

float easeInOutQuad(float t) 
{
    t /= 1.0f / 2.0f;
    if (t < 1) return 0.5f * t * t;
    t--;
    return -0.5f * (t * (t - 2) - 1);
}

void onServoInputWebSocketEvent(AsyncWebSocket *server, 
                      AsyncWebSocketClient *client, 
                      AwsEventType type,
                      void *arg, 
                      uint8_t *data, 
                      size_t len) 
{                      
  switch (type) 
  {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      panServo.write(0);
      tiltServo.write(90);
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      panServo.write(0);
      tiltServo.write(90);
      ledcWrite(PWMLightChannel, 0);
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
      {
        std::string myData = "";
        myData.assign((char *)data, len);
        Serial.printf("Key,Value = [%s]\n", myData.c_str());        
        
        if (myData == "ResetServos")
        {
            panServo.write(0);
            tiltServo.write(90);
        }
        else if (myData == "DemoServos")
        {
          int panPositions[] = {0, 130, 65, 0, 130, 20, 110, 45, 85, 0, 130, 30, 100, 55, 75, 0, 0};
          int tiltPositions[] = {0, 180, 90, 180, 0, 170, 10, 160, 20, 150, 30, 140, 40, 130, 50, 120, 90};

          int numPositions = sizeof(panPositions) / sizeof(panPositions[0]);

          for (int i = 0; i < numPositions; i++) 
          {
              panServo.write(panPositions[i]);
              tiltServo.write(tiltPositions[i]);

              delay(200); // wait for 200ms before moving to the next position
          }
        }

        else 
        {
            std::istringstream ss(myData);
            std::string key, value;
            std::getline(ss, key, ',');
            std::getline(ss, value, ',');
            if ( value != "" )
            {
                int valueInt = atoi(value.c_str());
                if (key == "Pan")
                {
                    panServo.write(valueInt);
                }
                else if (key == "Tilt")
                {
                    tiltServo.write(valueInt);   
                }
                else if (key == "Light")
                {
                    ledcWrite(PWMLightChannel, valueInt);    
                }           
            }
        }
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;   
  }
}

void onCameraWebSocketEvent(AsyncWebSocket *server, 
                      AsyncWebSocketClient *client, 
                      AwsEventType type,
                      void *arg, 
                      uint8_t *data, 
                      size_t len) 
{                      
  switch (type) 
  {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      cameraClientId = client->id();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      cameraClientId = 0;
      break;
    case WS_EVT_DATA:
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}

void setupCamera()
{
  camera_config_t config;
  config_camera(&config);

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) 
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }  

  if (psramFound())
  {
    heap_caps_malloc_extmem_enable(20000);  
    Serial.printf("PSRAM initialized. malloc to take memory from psram above this size");    
  }  

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_HQVGA);
}

void sendCameraPicture()
{
  if (cameraClientId == 0)
  {
    return;
  }
  unsigned long  startTime1 = millis();
  //capture a frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) 
  {
      Serial.println("Frame buffer could not be acquired");
      return;
  }

  unsigned long  startTime2 = millis();
  wsCamera.binary(cameraClientId, fb->buf, fb->len);
  esp_camera_fb_return(fb);
    
  //Wait for message to be delivered
  while (true)
  {
    AsyncWebSocketClient * clientPointer = wsCamera.client(cameraClientId);
    if (!clientPointer || !(clientPointer->queueIsFull()))
    {
      break;
    }
    delay(1);
  }
  
  unsigned long  startTime3 = millis();  
  //Serial.printf("Time taken Total: %d|%d|%d\n",startTime3 - startTime1, startTime2 - startTime1, startTime3-startTime2 );
}

void setUpPinModes()
{
  //dummyServo1.attach(DUMMY_SERVO1_PIN);
  //dummyServo2.attach(DUMMY_SERVO2_PIN);  
  panServo.attach(PAN_PIN);
  tiltServo.attach(TILT_PIN);

  panServo.write(0);
  tiltServo.write(90);
  delay(2000);

  //Set up flash light
  ledcSetup(PWMLightChannel, 1000, 8);
  pinMode(LIGHT_PIN, OUTPUT);    
  ledcAttachPin(LIGHT_PIN, PWMLightChannel);
}

void setup(void) 
{
  setUpPinModes();
  Serial.begin(115200);

  // Create an access point in the ESP32
  // WiFi.softAP(ssid, password);
  // IPAddress IP = WiFi.softAPIP();
  // Serial.print("AP IP address: ");
  // Serial.println(IP);

  // Connect to an existing wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

/*
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
*/
  setupCamera();
      
  wsCamera.onEvent(onCameraWebSocketEvent);
  server.addHandler(&wsCamera);

  wsServoInput.onEvent(onServoInputWebSocketEvent);
  server.addHandler(&wsServoInput);

  server.begin();
  Serial.println("HTTP server started");
}


void loop() 
{
  wsCamera.cleanupClients(); 
  wsServoInput.cleanupClients(); 
  sendCameraPicture(); 
  //Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());
}
