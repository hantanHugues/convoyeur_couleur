#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <TimeLib.h>

// Initialisation du capteur avec les paramètres par défaut
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

byte gammatable[256];

void setup() {
  Serial.begin(9600);
  Serial.println("Color Detection Test!");

  // Vérification du capteur
  if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1);
  }

  // Création de la table gamma
  for (int i = 0; i < 256; i++) {
    float x = i;
    x /= 255;
    x = pow(x, 2.5);
    x *= 255;
    gammatable[i] = (byte)x;
  }

  // Initialisation de l'heure (10 juillet 2025, 23:06:00 WAT)
  setTime(23, 6, 0, 10, 7, 2025);
}

void loop() {
  uint16_t clear, red, green, blue;

  // Lecture des données brutes
  tcs.getRawData(&red, &green, &blue, &clear);

  // Affichage de l'heure
  Serial.print("Time: ");
  Serial.print(hour()); Serial.print(":");
  if (minute() < 10) Serial.print("0"); Serial.print(minute()); Serial.print(":");
  if (second() < 10) Serial.print("0"); Serial.print(second());
  Serial.print(" | ");

  // Affichage des valeurs brutes
  Serial.print("C:\t"); Serial.print(clear);
  Serial.print("\tR:\t"); Serial.print(red);
  Serial.print("\tG:\t"); Serial.print(green);
  Serial.print("\tB:\t"); Serial.print(blue);

  // Normalisation des valeurs RVB
  float r, g, b;
  if (clear > 0) {
    r = red / (float)clear;
    g = green / (float)clear;
    b = blue / (float)clear;
  } else {
    r = g = b = 0;
  }

  // Mise à l'échelle et application de la correction gamma
  r *= 255; g *= 255; b *= 255;
  r = gammatable[(int)r];
  g = gammatable[(int)g];
  b = gammatable[(int)b];

  // Affichage des valeurs RVB corrigées (en hexadécimal)
  Serial.print("\t");
  Serial.print((int)r, HEX); Serial.print(" ");
  Serial.print((int)g, HEX); Serial.print(" ");
  Serial.print((int)b, HEX); Serial.print(" ");

  // Classification des couleurs
  String color;
  if (r > g && r > b && r > 50) {
    color = "Red";
  } else if (b > r && b > g && b > 50) {
    color = "Blue";
  } else if (g > r && g > b && g > 50 && r > 50) {
    color = "Yellow"; // Jaune = vert + rouge
  } else if (g > r && g > b && g > 50) {
    color = "Green";
  } else {
    color = "Unknown";
  }

  // Affichage de la couleur détectée
  Serial.print("\tColor: "); Serial.println(color);

  delay(1000); // Attendre 1 seconde avant la prochaine lecture
}


/*
* TODO:
*   1 - Ajustez les seuils de classification (par exemple, 50) en fonction de vos conditions d'éclairage.
*   2 - Remplacez TimeLib par un module RTC (comme le DS3231) pour une heure précise.
*   3 - 
*/