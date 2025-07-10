#include <Wire.h>
#include "Adafruit_TCS34725.h"

// --- CONFIGURATION DES BROCHES ---
#define TRIG_PIN 9
#define ECHO_PIN 10

// --- INITIALISATION DES CAPTEURS ---
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_600MS, TCS34725_GAIN_60X);

#define SEUIL_SATURATION_GRIS 25 
#define SEUIL_VALEUR_NOIR 20
#define SEUIL_VALEUR_BLANC 85

void setup() {
  Serial.begin(9600);
  Serial.println("Systeme de detection Couleur + Distance (sans bibliotheque ultrason)");

  if (tcs.begin()) {
    Serial.println("Capteur de couleur trouve !");
  } else {
    Serial.println("Aucun capteur TCS34725 trouve... verifiez le cablage !");
    while (1);
  }

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  long distance = mesurerDistanceCm();
  uint16_t r, g, b, c;
  
  delay(610); 
  tcs.getRawData(&r, &g, &b, &c);

  String nomCouleur = getCouleurNom(r, g, b);

  // --- NOUVELLE SECTION D'AFFICHAGE ---
  Serial.print("Distance: ");
  if (distance > 0 && distance < 400) {
    Serial.print(distance);
    Serial.print(" cm");
  } else {
    Serial.print("Hors de portee");
  }
  
  // Affichage des valeurs de couleur, y compris 'C'
  Serial.print("  |  R: "); Serial.print(r);
  Serial.print("  G: "); Serial.print(g);
  Serial.print("  B: "); Serial.print(b);
  Serial.print("  C: "); Serial.print(c); // LIGNE AJOUTÉE POUR LA VALEUR C
  
  Serial.print("  |  ==> Couleur detectee: ");
  Serial.println(nomCouleur);
}

// La fonction mesurerDistanceCm() est expliquée ci-dessus et reste la même
long mesurerDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  long distance = duration / 58;
  return distance;
}

// Les fonctions getCouleurNom() et RgbToHsv() restent les mêmes
String getCouleurNom(uint16_t r, uint16_t g, uint16_t b) {
  uint16_t maxVal = max(r, max(g, b));
  if (maxVal < 50) return "Noir";

  byte r_8bit = map(r, 0, maxVal, 0, 255);
  byte g_8bit = map(g, 0, maxVal, 0, 255);
  byte b_8bit = map(b, 0, maxVal, 0, 255);

  float teinte, saturation, valeur;
  RgbToHsv(r_8bit, g_8bit, b_8bit, &teinte, &saturation, &valeur);

  saturation *= 100;
  valeur *= 100;

  if (saturation < SEUIL_SATURATION_GRIS) {
    if (valeur < SEUIL_VALEUR_NOIR) return "Noir";
    if (valeur > SEUIL_VALEUR_BLANC) return "Blanc";
    return "Gris";
  }

  if (teinte < 15) return "Rouge";
  if (teinte < 45) return "Orange";
  if (teinte < 75) return "Jaune";
  if (teinte < 150) return "Vert";
  if (teinte < 180) return "Cyan";
  if (teinte < 270) return "Bleu";
  if (teinte < 330) return "Magenta / Violet";
  
  return "Rouge";
}

void RgbToHsv(byte r, byte g, byte b, float *h, float *s, float *v) {
  float r_f = r / 255.0;
  float g_f = g / 255.0;
  float b_f = b / 255.0;
  float Cmax = max(r_f, max(g_f, b_f));
  float Cmin = min(r_f, min(g_f, b_f));
  float delta = Cmax - Cmin;

  if (delta == 0) *h = 0;
  else if (Cmax == r_f) *h = 60 * fmod(((g_f - b_f) / delta), 6);
  else if (Cmax == g_f) *h = 60 * (((b_f - r_f) / delta) + 2);
  else if (Cmax == b_f) *h = 60 * (((r_f - g_f) / delta) + 4);

  if (*h < 0) *h += 360;

  if (Cmax == 0) *s = 0;
  else *s = delta / Cmax;

  *v = Cmax;
}