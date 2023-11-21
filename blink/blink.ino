#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

#define DHTPIN 26
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE); 

int toggle=1;
void IRAM_ATTR toggleLED(){
  // Code for the blinking LED Part
  Serial.println("Toggle LED Called!!!");
  if (toggle==0){
    //blink red
    digitalWrite(32, 1);
    digitalWrite(33, 0);
  } else {
    //blink white
    digitalWrite(33, 1);
    digitalWrite(32, 0);
  }
  if (toggle==1){
        toggle=0;
      } else {
        toggle=1;
      }
}

//Wifi and MQTT broker data
const char* ssid = "WiFi-2.4-743C";
const char* password = "CU1gd6p12yRY";
const char* mqtt_server = "broker.emqx.io";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  // initialize digital pins for input and output.
  pinMode(32, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(27, INPUT);

  //For DH11 setup
  dht.begin();
  attachInterrupt(27, toggleLED, RISING);
  //Wifi and MQTT setup
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

 void dht_get_data(){
  delay(1000);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Echec reception");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print("%  Temperature: ");
  Serial.print(t);
  Serial.print("°C, ");
  Serial.print(f);
  Serial.println("°F");

  //Final string to return
  sprintf(msg, "Humidity: %.3f Temperature: %.3f", h, t);
}

// the loop function runs over and over again forever
void loop() {
  //For client connections
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 1000) {
    dht_get_data();
    lastMsg = now;
    client.publish("krishna_topic_2", msg);
  }
}