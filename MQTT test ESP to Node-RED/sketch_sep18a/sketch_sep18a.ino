#include <WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network
const char* ssid = "proyecto_autito_net";
const char* password = "1846mV3)";
const char* mqttServer = "192.168.137.126";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  Serial.println("Connecting to WiFi");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }

  Serial.println("Connected to WiFi");

  // Connect to MQTT
  client.setServer(mqttServer, mqttPort);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT");
    } else {
      Serial.print("Failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  // Publish a message to "test" topic
  client.publish("test", "Hello World");
}

void loop() {
  client.loop(); // Para mantener activa la conexion mqtt
  static unsigned long lastPublishTime = 0; // keep track of the last time we published
  unsigned long currentTime = millis(); // current time in milliseconds since the board started

  if (currentTime - lastPublishTime >= 1000) { // 5000 milliseconds (5 seconds) have passed
    client.publish("test", "Hello World"); // publish a message
    lastPublishTime = currentTime; // update the last time we published
  }
}


