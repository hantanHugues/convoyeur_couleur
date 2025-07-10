#include <Wire.h>
#include "Adafruit_TCS34725.h"

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

// --- Paramètres pour l'identification des couleurs ---
// Vous pouvez ajuster ces seuils pour améliorer la détection

// En dessous de cette saturation (0-100), la couleur est considérée comme un ton de gris.
#define SEUIL_SATURATION_GRIS 25 

// En dessous de cette valeur/luminosité (0-100), c'est considéré comme Noir.
#define SEUIL_VALEUR_NOIR 20

// Au-dessus de cette valeur/luminosité ET avec une faible saturation, c'est considéré comme Blanc.
#define SEUIL_VALEUR_BLANC 85

void setup() {
  Serial.begin(9600);
  Serial.println("Test du capteur de couleur avec identification");

  if (tcs.begin()) {
    Serial.println("Capteur trouve !");
  } else {
    Serial.println("Aucun capteur TCS34725 trouve... verifiez le cablage !");
    while (1);
  }
}

void loop() {
  uint16_t r, g, b, c;

  // Attend que la lecture soit complète (dépend du temps d'intégration)
  delay(60); 
  tcs.getRawData(&r, &g, &b, &c);

  // Affiche les valeurs brutes
  Serial.print("R: "); Serial.print(r);
  Serial.print("  G: "); Serial.print(g);
  Serial.print("  B: "); Serial.print(b);

  // Obtient et affiche le nom de la couleur
  String nomCouleur = getCouleurNom(r, g, b);
  Serial.print("  ==> Couleur detectee: ");
  Serial.println(nomCouleur);

  Serial.println("---------------------------------");

  delay(1000);
}

// Fonction qui convertit les valeurs RGB en un nom de couleur
String getCouleurNom(uint16_t r, uint16_t g, uint16_t b) {
  // Les valeurs du capteur sont élevées, on les ramène à une échelle 0-255
  // La fonction map() est parfaite pour ça.
  // Note: la valeur max du capteur dépend du gain et du temps d'intégration.
  // On va utiliser la valeur 'clear' (c) comme approximation de la pleine échelle
  // pour une meilleure normalisation. Si 'c' est 0, on utilise une valeur par défaut.
  uint16_t maxVal = max(r, max(g, b));
  if (maxVal == 0) return "Noir"; // Si tout est à 0

  byte r_8bit = map(r, 0, maxVal, 0, 255);
  byte g_8bit = map(g, 0, maxVal, 0, 255);
  byte b_8bit = map(b, 0, maxVal, 0, 255);

  // Conversion RGB vers HSV (Teinte, Saturation, Valeur)
  float teinte, saturation, valeur;
  RgbToHsv(r_8bit, g_8bit, b_8bit, &teinte, &saturation, &valeur);

  // On travaille avec des pourcentages pour la saturation et la valeur
  saturation *= 100;
  valeur *= 100;

  // 1. Détecter Noir, Blanc, Gris (couleurs achromatiques)
  if (saturation < SEUIL_SATURATION_GRIS) {
    if (valeur < SEUIL_VALEUR_NOIR) return "Noir";
    if (valeur > SEUIL_VALEUR_BLANC) return "Blanc";
    return "Gris";
  }

  // 2. Détecter les couleurs par leur teinte (Hue)
  if (teinte < 15) return "Rouge";
  if (teinte < 45) return "Orange";
  if (teinte < 75) return "Jaune";
  if (teinte < 150) return "Vert";
  if (teinte < 180) return "Cyan";
  if (teinte < 270) return "Bleu";
  if (teinte < 330) return "Magenta / Violet";
  
  return "Rouge"; // Par défaut (pour la plage 330-360)
}


// Algorithme standard de conversion RGB (0-255) en HSV (H:0-360, S:0-1, V:0-1)
void RgbToHsv(byte r, byte g, byte b, float *h, float *s, float *v) {
  float r_f = r / 255.0;
  float g_f = g / 255.0;
  float b_f = b / 255.0;

  float Cmax = max(r_f, max(g_f, b_f));
  float Cmin = min(r_f, min(g_f, b_f));
  float delta = Cmax - Cmin;

  // Calcul de la Teinte (Hue)
  if (delta == 0) {
    *h = 0;
  } else if (Cmax == r_f) {
    *h = 60 * fmod(((g_f - b_f) / delta), 6);
  } else if (Cmax == g_f) {
    *h = 60 * (((b_f - r_f) / delta) + 2);
  } else if (Cmax == b_f) {
    *h = 60 * (((r_f - g_f) / delta) + 4);
  }

  if (*h < 0) {
    *h += 360;
  }

  // Calcul de la Saturation
  if (Cmax == 0) {
    *s = 0;
  } else {
    *s = delta / Cmax;
  }

  // Calcul de la Valeur (Value)
  *v = Cmax;
}