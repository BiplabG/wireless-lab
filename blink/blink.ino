#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#define DHTPIN 26
#define DHTTYPE DHT11
#define DEBOUNCE_TIME  50

unsigned long lastDebounceTime = 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
DHT dht(DHTPIN, DHTTYPE); 

int toggle=1;
void IRAM_ATTR toggleLED(){
  if ((millis() - lastDebounceTime) > DEBOUNCE_TIME ){
    if (toggle==0){
      //blink red
      Serial.println("Stopped collecting data.");
      digitalWrite(32, 1);
      digitalWrite(33, 0);
    } else {
      //blink white
      Serial.println("Resumed collecting data.");
      digitalWrite(33, 1);
      digitalWrite(32, 0);
    }
    if (toggle==1){
          toggle=0;
        } else {
          toggle=1;
        }
    lastDebounceTime = millis();
  }
}

//Wifi and MQTT broker data
const char* ssid = "WiFi-2.4-743C";
const char* password = "CU1gd6p12yRY";
const char* mqtt_server = "broker.emqx.io";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[100];
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

  // NTPClient to get time
  timeClient.begin();
  timeClient.setTimeOffset(3600);
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
  String time = timeClient.getFormattedDate();
  char timestamp[time.length() + 1];
  time.toCharArray(timestamp, time.length() + 1);

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
  Serial.print("°F");
  Serial.print("  TimeStamp: ");
  Serial.println(timestamp);

  //Final string to return // Resolution is 1 
  sprintf(msg, "Humidity: %.f Temperature: %.f TimeStamp: %s", h, t, timestamp);
}

// the loop function runs over and over again forever
void loop() {
  //For client connections
  if (!client.connected()) {
    reconnect();
  }
  //Ensure we get valid date and time
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  client.loop();
  
  long now = millis();
  if (now - lastMsg > 1000 && toggle == 0) {
    dht_get_data();
    lastMsg = now;
    client.publish("krishna_topic_2", msg);
  }
}