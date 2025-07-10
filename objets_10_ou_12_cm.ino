#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <TimeLib.h>

// Initialisation du capteur avec un temps d'intégration court et un gain élevé
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_60X);

byte gammatable[256];

void setup() {
  Serial.begin(9600);
  Serial.println("Color Detection Test for Conveyor!");

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

  // Initialisation de l'heure (10 juillet 2025, 23:13:00 WAT)
  setTime(23, 13, 0, 10, 7, 2025);
}

String classifyColor(float r, float g, float b) {
  if (r > 0.5 && g < 0.3 && b < 0.3) return "Red";
  if (b > 0.5 && r < 0.3 && g < 0.3) return "Blue";
  if (g > 0.4 && r > 0.3 && b < 0.2) return "Yellow";
  if (g > 0.5 && r < 0.3 && b < 0.3) return "Green";
  return "Unknown";
}

void loop() {
  uint16_t clear, red, green, blue;

  // Prendre plusieurs lectures pour réduire le bruit
  uint16_t r_sum = 0, g_sum = 0, b_sum = 0, c_sum = 0;
  int num_reads = 5;
  for (int i = 0; i < num_reads; i++) {
    tcs.getRawData(&red, &green, &blue, &clear);
    r_sum += red; g_sum += green; b_sum += blue; c_sum += clear;
    delay(5);
  }
  red = r_sum / num_reads; green = g_sum / num_reads;
  blue = b_sum / num_reads; clear = c_sum / num_reads;

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
  String color = classifyColor(r / 255.0, g / 255.0, b / 255.0);
  Serial.print("\tColor: "); Serial.println(color);

  delay(500); // Attendre 0.5 seconde avant la prochaine lecture
}

/*
*Placez des objets de couleur rouge, bleu, jaune et vert à 10-12 cm du capteur dans les conditions d’éclairage du convoyeur.
*Si les lectures sont instables, augmentez num_reads ou ajustez le temps d'intégration/gain.
*
*Si la vitesse du convoyeur est connue (par exemple, 10 cm/s), calculez le temps pendant lequel un objet reste dans la zone de détection (par exemple, un objet de 5 cm de large prend 0.5 s à passer).
Ajustez le delay dans loop() pour correspondre à ce temps ou utilisez un capteur de proximité pour déclencher les lectures.

le temps d'intégration etait de 50 ms (TCS34725_INTEGRATIONTIME_50MS). Pour un convoyeur rapide, réduisez ce temps (par exemple, à TCS34725_INTEGRATIONTIME_2_4MS) pour des lectures plus rapides, mais augmentez le gain (par exemple, TCS34725_GAIN_60X) pour compenser la perte de lumière à 10-12 cm.
*/