#include <Wire.h>
#include "Adafruit_TCS34725.h"

// --- PARAMÈTRES DE CONFIGURATION ---
// Tu peux ajuster ces valeurs si nécessaire
#define HUE_TOLERANCE 15             // Tolérance autour de la teinte apprise (en degrés, 10-20 est une bonne valeur)
#define SATURATION_THRESHOLD 20.0f   // Seuil de saturation pour distinguer une couleur d'un gris/blanc/noir (en %)

// --- VARIABLES GLOBALES POUR LA CALIBRATION ---
// Facteurs de correction pour la balance des blancs
float cal_R_factor = 1.0f;
float cal_G_factor = 1.0f;
float cal_B_factor = 1.0f;

// Teintes (Hue) de référence apprises pendant la calibration
float ref_hue_red = 0.0f;
float ref_hue_green = 120.0f;
float ref_hue_blue = 240.0f;
float ref_hue_yellow = 60.0f;

// Initialisation du capteur
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_16X); // Gain 16X est un bon compromis


void setup() {
  Serial.begin(115200); // Utilise une vitesse de 115200 baud
  // Attends que le moniteur série soit ouvert (utile pour les Arduinos comme le Leonardo, mais ne nuit pas aux autres)
  while (!Serial) {
    delay(10);
  }

  Serial.println("\nColor Detection with Advanced Calibration (Serial Control)");

  if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1);
  }

  // --- ROUTINE DE CALIBRATION INTERACTIVE ---
  Serial.println("\n--- CALIBRATION ---");
  Serial.println("Voulez-vous lancer la procédure de calibration ?");
  Serial.println("Envoyez 'o' ou 'oui' pour calibrer, ou attendez 5 secondes pour ignorer.");

  // Attend une entrée série ou un timeout de 5 secondes
  bool calibrationRequested = false;
  long startTime = millis();
  while (millis() - startTime < 5000) {
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      if (input.equalsIgnoreCase("o") || input.equalsIgnoreCase("oui")) {
        calibrationRequested = true;
      }
      break;
    }
  }

  if (calibrationRequested) {
    runCalibration(); // Lance la procédure de calibration complète
  } else {
    Serial.println("\nTimeout ou réponse négative, utilisation des valeurs de calibration par défaut.");
  }

  Serial.println("\nCalibration terminée. Démarrage de la détection de couleur...");
}


void loop() {
  // 1. Lire les données brutes du capteur
  uint16_t r_raw, g_raw, b_raw, c_raw;
  tcs.getRawData(&r_raw, &g_raw, &b_raw, &c_raw);

  // 2. Appliquer la correction de la balance des blancs
  float r_cal = r_raw * cal_R_factor;
  float g_cal = g_raw * cal_G_factor;
  float b_cal = b_raw * cal_B_factor;

  // 3. Normaliser les valeurs par la luminosité (Clear)
  float r_norm = (c_raw > 0) ? (float)r_cal / c_raw : 0;
  float g_norm = (c_raw > 0) ? (float)g_cal / c_raw : 0;
  float b_norm = (c_raw > 0) ? (float)b_cal / c_raw : 0;

  // 4. Convertir en HSV
  float hue, saturation, value;
  RgbToHsv(r_norm, g_norm, b_norm, hue, saturation, value);

  // 5. Identifier la couleur
  String detectedColor = identifyColor(hue, saturation, value);
  
  // 6. Afficher les résultats
  Serial.print("Hue: "); Serial.print(hue, 1);
  Serial.print("\tSat: "); Serial.print(saturation, 1);
  Serial.print("\tVal: "); Serial.print(value, 1);
  Serial.print("\t---> Couleur Détectée: "); Serial.println(detectedColor);

  delay(500);
}

// =========================================================================
// ==================== FONCTIONS DE CALIBRATION ===========================
// =========================================================================

void runCalibration() {
  // --- ÉTAPE 1: BALANCE DES BLANCS ---
  Serial.println("\n[ÉTAPE 1/5] Balance des Blancs");
  Serial.println("Placez une feuille de papier BLANCHE devant le capteur et envoyez 'ok' (ou autre chose) puis Entrée.");
  waitForSerialInput();
  
  uint16_t r_white, g_white, b_white, c_white;
  tcs.getRawData(&r_white, &g_white, &b_white, &c_white);
  
  // Calcule les facteurs de correction
  float max_val = max(r_white, max(g_white, b_white));
  cal_R_factor = (r_white > 0) ? max_val / r_white : 1.0f;
  cal_G_factor = (g_white > 0) ? max_val / g_white : 1.0f;
  cal_B_factor = (b_white > 0) ? max_val / b_white : 1.0f;

  Serial.println("Balance des blancs terminée.");
  delay(500);

  // --- ÉTAPES 2-5: APPRENTISSAGE DES COULEURS DE RÉFÉRENCE ---
  ref_hue_red = learnColorHue("ROUGE");
  ref_hue_green = learnColorHue("VERT");
  ref_hue_blue = learnColorHue("BLEU");
  ref_hue_yellow = learnColorHue("JAUNE");
}

float learnColorHue(String colorName) {
  Serial.println("\n[Apprentissage] Calibrage pour " + colorName);
  Serial.println("Placez un objet " + colorName + " devant le capteur et envoyez 'ok'.");
  waitForSerialInput();

  uint16_t r_raw, g_raw, b_raw, c_raw;
  tcs.getRawData(&r_raw, &g_raw, &b_raw, &c_raw);
  
  float r_cal = r_raw * cal_R_factor;
  float g_cal = g_raw * cal_G_factor;
  float b_cal = b_raw * cal_B_factor;
  
  float r_norm = (c_raw > 0) ? (float)r_cal / c_raw : 0;
  float g_norm = (c_raw > 0) ? (float)g_cal / c_raw : 0;
  float b_norm = (c_raw > 0) ? (float)b_cal / c_raw : 0;

  float hue, sat, val;
  RgbToHsv(r_norm, g_norm, b_norm, hue, sat, val);
  
  Serial.println(colorName + " appris avec une Teinte (Hue) de: " + String(hue));
  delay(500);
  return hue;
}

void waitForSerialInput() {
  // Vide le buffer série de tout résidu
  while(Serial.available() > 0) {
    Serial.read();
  }
  // Attend une nouvelle entrée de l'utilisateur
  while (Serial.available() == 0) {
    // Boucle d'attente
  }
}

// =========================================================================
// ==================== FONCTIONS DE DÉTECTION =============================
// =========================================================================

String identifyColor(float hue, float saturation, float value) {
  if (saturation < SATURATION_THRESHOLD) {
    if (value > 80.0f) return "Blanc";
    if (value < 10.0f) return "Noir";
    return "Gris";
  }
  if (isClose(hue, ref_hue_red, HUE_TOLERANCE)) return "Rouge";
  if (isClose(hue, ref_hue_green, HUE_TOLERANCE)) return "Vert";
  if (isClose(hue, ref_hue_blue, HUE_TOLERANCE)) return "Bleu";
  if (isClose(hue, ref_hue_yellow, HUE_TOLERANCE)) return "Jaune";
  return "Inconnue";
}

bool isClose(float hue, float ref_hue, float tolerance) {
  float diff = abs(hue - ref_hue);
  if (diff < tolerance) return true;
  float diff_wrap = 360.0f - diff;
  if (diff_wrap < tolerance) return true;
  return false;
}

// =========================================================================
// ============= FONCTION DE CONVERSION RGB vers HSV =======================
// =========================================================================

void RgbToHsv(float r, float g, float b, float &h, float &s, float &v) {
  float cmax = max(r, max(g, b));
  float cmin = min(r, min(g, b));
  float diff = cmax - cmin;
  
  if (cmax == cmin) { h = 0; }
  else if (cmax == r) { h = fmod(60 * ((g - b) / diff) + 360, 360); }
  else if (cmax == g) { h = fmod(60 * ((b - r) / diff) + 120, 360); }
  else if (cmax == b) { h = fmod(60 * ((r - g) / diff) + 240, 360); }
  
  s = (cmax == 0) ? 0 : (diff / cmax) * 100;
  v = cmax * 100;
}