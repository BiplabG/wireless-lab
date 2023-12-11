#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Crypto.h>
#include <AES.h>
#include <string.h>

#define DHTPIN 26
#define DHTTYPE DHT11
#define DEBOUNCE_TIME  50

unsigned long lastDebounceTime = 0;
int toggle=1;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
DHT dht(DHTPIN, DHTTYPE); 

// For AES Encryption
byte key[16]={0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
AES128 aes128;
char encryptedText[128];
char decryptedText[128];

void encryptWhole(char *plain, size_t length){
  int byte_len;
  if (length % 16 == 0){
    byte_len = length;
  } else {
    byte_len = (length / 16) * 16 + 16;
  }
  byte plainbyte[byte_len];
  for (int i=0; i < byte_len-1; i++){
    if (i >= length){
      plainbyte[i] = 0x0;
    }
    else {
      plainbyte[i] = plain[i];
    }
  }
  byte cypher[byte_len];

  byte *cp;
  byte *pbp;
  cp = cypher;
  pbp = plainbyte;
  for (int j=0; j<byte_len; j=j+16){
    aes128.encryptBlock(cp + j, pbp + j);
  }
  // char encryptedText[byte_len];
  for (int i=0; i < byte_len; i++){
      encryptedText[i] = cypher[i];
  }
}

void decryptWhole(char *encryptedText, size_t length){
  int byte_len;
  if (length % 16 == 0){
    byte_len = length;
  } else {
    byte_len = (length / 16) * 16 + 16;
  }

  byte cypher[byte_len];
  for (int i=0; i < byte_len; i++){
    cypher[i] = encryptedText[i];
  }

  byte decryptedByte[byte_len];
  byte *dcp;
  byte *dpbp;
  dcp = cypher;
  dpbp = decryptedByte;
  for (int j=0; j<byte_len; j=j+16){
    aes128.decryptBlock(dpbp+j, dcp+j);
  }
  for (int i=0; i < byte_len-1; i++){
      decryptedText[i] = decryptedByte[i];
  }
}



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
const char* ssid = "WiFi-2.4-E678";
const char* password = "ws5rm27kjcu9s";
const char* mqtt_server = "192.168.1.40";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
long lastheartbeat = 0;
bool sendData = false;
char msg[200];
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

  //AES Setup
  aes128.setKey(key,16);// Setting Key for AES
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
  
  String messageTemp;
  if (strcmp(topic, "krishna_topic_2") == 0){
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    for (int i = 0; i < length; i++) {
      Serial.print((char)message[i]);
      messageTemp += (char)message[i];
    }
  } else if(strcmp(topic, "heartbeat") == 0){
    lastheartbeat = millis();
    sendData = true;
    Serial.print("Heartbeat received. Server is active.");
  } else if(strcmp(topic, "acknowledge") == 0){
    sendData = true;
    Serial.print("Acknowledgment received. Start sending data.");
  }  
  
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    sendData = false;
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
      client.subscribe("acknowledge");
      client.subscribe("heartbeat");
      client.publish("start", "Initial start message");
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
  // sprintf(msg, "Humidity: %.f Temperature: %.f TimeStamp: %s", h, t, timestamp);

  sprintf(msg, "Humidity: %.f Temperature: %.f TimeStamp: %s Hash: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", h, t, timestamp, hash_buffer[0],hash_buffer[1],hash_buffer[2],hash_buffer[3],hash_buffer[4],hash_buffer[5],hash_buffer[6],hash_buffer[7],hash_buffer[8],hash_buffer[9],hash_buffer[10],hash_buffer[11],hash_buffer[12],hash_buffer[13],hash_buffer[14],hash_buffer[15],hash_buffer[16],hash_buffer[17],hash_buffer[18],hash_buffer[19],hash_buffer[20],hash_buffer[21],hash_buffer[22],hash_buffer[23],hash_buffer[24],hash_buffer[25],hash_buffer[26],hash_buffer[27],hash_buffer[28],hash_buffer[29],hash_buffer[30],hash_buffer[31]);

  Serial.println(msg);
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
  if (now - lastMsg > 1000 && toggle == 0 && sendData) {
    dht_get_data();
    lastMsg = now;
    encryptWhole(msg, strlen(msg));
    client.publish("krishna_topic_2", encryptedText);
  }
  if (now - lastheartbeat > 30000){
    Serial.println(sendData);
    client.publish("start", "Initial start message");
    if (!client.connected()) {
      reconnect();
    }
    else if(!sendData){
      client.publish("start", "Initial start message");
    }
  }
}