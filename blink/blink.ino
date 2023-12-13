#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Crypto.h>
#include <AES.h>
#include <string.h>
#include "BLAKE2s.h"

#define DHTPIN 26
#define DHTTYPE DHT11
#define DEBOUNCE_TIME  50

unsigned long lastDebounceTime = 0;
int toggle=1;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
DHT dht(DHTPIN, DHTTYPE); 

//Wifi and MQTT broker data
const char* ssid = "biplabph";
const char* password = "biplab123";
const char* mqtt_server = "192.168.215.75";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
long lastheartbeat = 0;
bool sendData = true;
char msg[512];
int value = 0;

// For AES Encryption
byte key[16]={0x0A, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
AES128 aes128;
char encryptedText[512];
char decryptedText[512];

// For hashing part
BLAKE2s blake2s;
byte hash_key[16]={0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};


void encryptWhole(char *plain, size_t length){
  // Assuming aes128.encryptBlock() encrypts 16 bytes at a time
    const size_t block_size = 16;
    size_t padded_length = length + (block_size - (length % block_size)); // Calculate padded length
    uint8_t *padded_plain = (uint8_t *)malloc(padded_length * sizeof(uint8_t)); // Allocate memory for padded plaintext

    // Copy the original plaintext and pad it
    memcpy(padded_plain, plain, length);
    memset(padded_plain + length, padded_length - length, padded_length - length);

    // Calculate the number of blocks
    size_t num_blocks = padded_length / block_size;
    uint8_t *cipher = (uint8_t *)malloc(num_blocks * block_size * sizeof(uint8_t)); // Allocate memory for ciphertext

    // Encrypt each block and store the cipher
    for (size_t i = 0; i < num_blocks; ++i) {
        aes128.encryptBlock(&cipher[i * block_size], &padded_plain[i * block_size]);
    }

    // Convert cipher blocks to a string (hex representation)
    // char encryptedText[num_blocks * block_size * 2 + 1]; // +1 for null terminator
    size_t index = 0;
    for (size_t i = 0; i < num_blocks * block_size; ++i) {
        sprintf(&encryptedText[index], "%02x", cipher[i]);
        index += 2;
    }
    encryptedText[index] = '\0'; // Add null terminator

    // Output the encrypted text
    // printf("Encrypted Text: %s\n", encryptedText);

    // Free dynamically allocated memory
    free(padded_plain);
    free(cipher);

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
          sendData = true;  
          lastheartbeat = millis(); 
        } else {
          toggle=1;
        }
    lastDebounceTime = millis();
  }
}


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
  if (strcmp(topic, "krishna_topic_1") == 0){
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    for (int i = 0; i < length; i++) {
      Serial.print((char)message[i]);
      messageTemp += (char)message[i];
    }
  } else if(strcmp(topic, "acknowledge1") == 0){
    for (int i = 0; i < length; i++) {
      messageTemp += (char)message[i];
    }
    lastheartbeat = millis();
    sendData = true;
    Serial.print(messageTemp);
  }  
  
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client1")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("acknowledge1");
      //client.publish("heartbeat", "Initial start message");
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

  char temp_msg[128];
  byte hash_buffer[128];
  sprintf(temp_msg, "Humidity: %.f Temperature: %.f TimeStamp: %s", h, t, timestamp);
  encryptWhole(temp_msg, strlen(temp_msg));
  Serial.println(temp_msg);
  
  blake2s.reset();
  blake2s.update(temp_msg, strlen(temp_msg));
  blake2s.finalize(hash_buffer, 32);
  blake2s.clear();
  //Final string to return // Resolution is 1 
  // sprintf(msg, "Humidity: %.f Temperature: %.f TimeStamp: %s", h, t, timestamp);
  char hash_string[33];
  size_t index = 0;
  for (size_t i = 0; i < 32; ++i) {
        sprintf(&hash_string[index], "%02x", hash_buffer[i]);
        index += 2;
    }

  strncat(encryptedText, " Hash: ", 7);
  strncat(encryptedText, hash_string, strlen(hash_string));
  
  Serial.println(encryptedText);
}

// the loop function runs over and over again forever
void loop() {
  //For client connections
  if (!client.connected()) {
    reconnect();
    lastheartbeat = millis();
  }
  //Ensure we get valid date and time
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  client.loop();
  
  long now = millis();
  if (now - lastMsg > 5000 && toggle == 0 && sendData) {
    dht_get_data();
    lastMsg = now;
    // encryptWhole(msg, strlen(msg));
    client.publish("krishna_topic_1", encryptedText);
  }
  if (now - lastheartbeat > 60000){
    sendData = false;
    Serial.println(" Server is not alive. Stop sending data");
    if (!client.connected()) {
      reconnect();
    }
    lastheartbeat = now;
  }
}