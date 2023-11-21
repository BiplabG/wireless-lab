#include "DHT.h"

#define DHTPIN 26
#define DHTTYPE DHT11
//DHTTYPE = DHT11, mais il existe aussi le DHT22 et 21

DHT dht(DHTPIN, DHTTYPE); // constructeur pour déclarer notre capteur

void setup() {
  Serial.begin(9600);
  dht.begin();
}

void loop() {
  delay(1000);
  // Le DHT11 renvoie au maximum une mesure toute les 1s
  float h = dht.readHumidity();
  //Lis le taux d'humidite en %
  float t = dht.readTemperature();
  //Lis la température en degré celsius
  float f = dht.readTemperature(true);
  //Avec (true), renvoi la température en fahrenheit

  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Echec reception");
    return;
    //Renvoie une erreur si l'ESP32 ne reçoit aucune mesure
  }

  Serial.print("Humidite: ");
  Serial.print(h);
  Serial.print("%  Temperature: ");
  Serial.print(t);
  Serial.print("°C, ");
  Serial.print(f);
  Serial.println("°F");
  // Transmet les mesures reçues dans le moniteur série
}