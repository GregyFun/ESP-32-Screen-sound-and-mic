# ESP32-S3 + ILI9488 IPS Display - Test Réussi ✅

## Vue d'ensemble

Projet de test **validé et fonctionnel** pour l'affichage sur écran **ILI9488 IPS 480x320** avec un ESP32-S3.

## Matériel

- **Microcontrôleur**: ESP32-S3-N16R8 (16MB Flash, 8MB PSRAM Octal)
- **Écran**: ILI9488 480x320 IPS TFT LCD (SPI)
- **Touch**: FT6236 (I2C) - Non implémenté
- **SD Card**: Lecteur SD intégré (partage le bus SPI avec LCD) - Non implémenté
- **Total pins**: 14 (VCC, GND, 6 SPI, 4 I2C touch, 2 contrôle)

## ⚠️ IMPORTANT - Lisez WIRING_GUIDE.md en premier!

**Avant de commencer**, consultez le fichier [WIRING_GUIDE.md](./WIRING_GUIDE.md) qui contient:
- La liste complète des 14 pins de l'écran avec correspondances GPIO
- Les points critiques spécifiques aux écrans **IPS** (différents des TN)
- La procédure de câblage complète
- Les paramètres d'initialisation critiques (RGB666, inversion, VCOM)
- Le dépannage complet

## Configuration des Pins (TESTÉE ET VALIDÉE ✅)

### Câblage minimal pour l'affichage LCD (7 fils):

```c
// LCD SPI Pins - CONFIGURATION QUI FONCTIONNE
#define PIN_NUM_MOSI   11    // SDI → Pin 9 de l'écran
#define PIN_NUM_CLK    12    // SCK → Pin 8 de l'écran
#define PIN_NUM_CS     10    // LCD_CS → Pin 12 de l'écran
#define PIN_NUM_DC     9     // LCD_RS (Data/Command) → Pin 10 de l'écran
#define PIN_NUM_RST    14    // LCD_RST → Pin 11 de l'écran
// + VCC → 3.3V (Pin 14 de l'écran)
// + GND → GND (Pin 13 de l'écran)
```

### Pins optionnels (NON connectés dans cette version):

```c
// MISO non connecté - pas nécessaire pour l'affichage
// Backlight non connecté - l'écran s'allume automatiquement
// Touch pins - non implémentés (pour plus tard)
// SD Card - non implémenté (pour plus tard)
```

## Points Critiques ILI9488 IPS

### 🔴 CRITIQUE #1: Format couleur 18-bit (RGB666)
L'ILI9488 sur SPI **nécessite 18 bits par pixel** (3 octets), PAS 16 bits!
- Format pixel: `0x66` (RGB666)
- Chaque pixel = 3 octets: R(6-bit), G(6-bit), B(6-bit)

### 🔴 CRITIQUE #2: Inversion d'affichage pour IPS
Les écrans **IPS nécessitent l'inversion ON** (commande 0x21)
- Sans cela, l'écran reste blanc ou affiche des couleurs inversées

### 🔴 CRITIQUE #3: VCOM spécifique IPS
Les écrans IPS utilisent VCOM = `{0x00, 0x4D, 0x80}` (pas `0x22`)

### 🔴 CRITIQUE #4: Soft Reset obligatoire
La commande 0x01 (Software Reset) doit être la première commande envoyée

## Compilation et Flash

### Prérequis:
- ESP-IDF v5.5.1 ou ultérieur
- Port COM configuré (voir Device Manager Windows)

### Méthode 1: Extension VS Code ESP-IDF
1. Ouvrir ce dossier dans VS Code
2. `F1` → "ESP-IDF: Set Espressif device target" → `esp32s3`
3. `F1` → "ESP-IDF: Build your project"
4. Connecter l'ESP32-S3 via USB
5. `F1` → "ESP-IDF: Flash your project"
6. `F1` → "ESP-IDF: Monitor your device"

### Méthode 2: Ligne de commande (ESP-IDF PowerShell)

```powershell
# Première fois uniquement
idf.py set-target esp32s3

# Compilation
idf.py build

# Flash + Moniteur (remplacer COM4 par votre port)
idf.py -p COM4 flash monitor

# Pour quitter le moniteur: Ctrl+]
```

### Clean Build (si nécessaire):
```powershell
idf.py fullclean
idf.py build
```

## Résultat Attendu ✅

### Sur l'écran:
L'écran affiche successivement en boucle:
1. **ROUGE** (2 secondes)
2. **VERT** (2 secondes)
3. **BLEU** (2 secondes)

### Logs du moniteur série:
```
I (xxx) S3_DISPLAY: ESP32-S3 + ILI9488 Display Test - Direct SPI
I (xxx) S3_DISPLAY: Initializing SPI bus...
I (xxx) S3_DISPLAY: Initializing ILI9488 IPS...
I (xxx) S3_DISPLAY: ILI9488 IPS initialized
I (xxx) S3_DISPLAY: Filling screen with RED...
I (xxx) S3_DISPLAY: Filling screen with GREEN...
I (xxx) S3_DISPLAY: Filling screen with BLUE...
I (xxx) S3_DISPLAY: Display test complete!
```

## Configuration du Projet

### sdkconfig.defaults:
- Target: `esp32s3`
- Flash: 16MB (CONFIG_ESPTOOLPY_FLASHSIZE_16MB)
- PSRAM: 8MB Octal Mode @ 80MHz
- CPU: 240MHz
- Partition: Single App Large
- LVGL: Configuration de base (non utilisé pour l'instant)

### Dépendances (idf_component.yml):
```yaml
dependencies:
  lvgl/lvgl:
    version: "^8.3.0"
```
Note: LVGL est installé mais non utilisé dans cette version de test

## Dépannage

### L'écran reste blanc:
✅ **RÉSOLU** - Les problèmes suivants ont été résolus:
- Format couleur 16-bit → **Changé pour 18-bit (RGB666)**
- Inversion OFF → **Activée (commande 0x21)**
- VCOM TN (0x22) → **VCOM IPS (0x4D)**
- Pas de soft reset → **Ajouté (commande 0x01)**

### Erreur "Wrong chip" lors du flash:
- Vérifier que le target est bien `esp32s3`
- Exécuter `idf.py set-target esp32s3`
- Si nécessaire: supprimer `sdkconfig` et recompiler

### Warning "Flash size mismatch":
- Normal si vous voyez ce message une seule fois
- Résolu en définissant `CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y`

### Lecture ID retourne 0xFF:
- **Normal** - MISO non connecté, pas nécessaire pour l'affichage

## Prochaines Étapes

### Phase 1: Affichage de base ✅
- [x] Configuration SPI
- [x] Initialisation ILI9488 IPS
- [x] Remplissage écran avec couleurs RGB666
- [x] Test rouge/vert/bleu

### Phase 2: Affichage de texte ✅
- [x] Intégration LVGL
- [x] Créer un driver d'affichage LVGL
- [x] Afficher "Hello World" au centre

### Phase 3: Audio I2S ✅
- [x] Configuration I2S pour MAX98357A
- [x] Test audio avec tonalités
- [x] Validation du hardware audio

### Phase 4: WiFi ✅
- [x] Configuration WiFi station mode
- [x] Connexion au réseau
- [x] Obtention d'une adresse IP

### Phase 5: Text-to-Speech (En cours) ⏳
- [x] Configuration WiFi
- [ ] Obtenir clé API Google Cloud TTS
- [ ] Implémenter client HTTP pour Google TTS
- [ ] Décoder MP3 audio
- [ ] Intégrer TTS → Audio playback

### Phase 6: Touch (À faire)
- [ ] Initialisation FT6236 (I2C)
- [ ] Lecture des coordonnées tactiles
- [ ] Intégration LVGL touch driver

### Phase 7: SD Card (À faire)
- [ ] Initialisation lecteur SD
- [ ] Test lecture/écriture
- [ ] Chargement d'images depuis SD

## Architecture du Code

```
main/
├── main.c              # Code principal avec initialisation ILI9488
├── idf_component.yml   # Dépendances (LVGL)
└── lv_conf.h          # Configuration LVGL (minimal)

sdkconfig.defaults      # Configuration ESP32-S3 + PSRAM + Flash
CMakeLists.txt         # Configuration CMake
WIRING_GUIDE.md        # Guide de câblage détaillé
README.md              # Ce fichier
```

## Spécifications Techniques

### SPI Configuration:
- **Host**: SPI2_HOST
- **Vitesse**: 4 MHz (max recommandé avec câbles Dupont)
- **Mode**: Mode 0 (CPOL=0, CPHA=0)
- **Type**: Half-duplex (unidirectionnel vers écran)
- **DMA**: Activé (SPI_DMA_CH_AUTO)

### ILI9488 IPS:
- **Résolution**: 480x320 pixels
- **Interface**: SPI 4-wire
- **Format couleur**: RGB666 (18-bit, 3 octets/pixel)
- **Inversion**: ON (obligatoire pour IPS)
- **VCOM**: -0.79688V (0x4D)
- **Fréquence**: 70Hz

### ESP32-S3:
- **Flash**: 16MB
- **PSRAM**: 8MB Octal @ 80MHz
- **CPU**: Dual-core Xtensa LX7 @ 240MHz
- **USB**: Native USB pour flash/debug
- **IDE**: ESP-IDF v5.5.1

## Ressources Utiles

### Documentation:
- [ESP-IDF SPI Master Driver](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/spi_master.html)
- [ILI9488 Datasheet](https://www.displayfuture.com/Display/datasheet/controller/ILI9488.pdf)
- [LVGL Documentation](https://docs.lvgl.io/8.3/)

### Recherches qui ont résolu le problème:
- [ESP32 with ILI9488 White screen - TFT_eSPI #2309](https://github.com/Bodmer/TFT_eSPI/discussions/2309)
- [ILI9488 IPS Display initialization - LovyanGFX #449](https://github.com/lovyan03/LovyanGFX/discussions/449)
- [Need sample code for ILI9488 LCD on SPI Interface - ESP32 Forum](https://esp32.com/viewtopic.php?t=1683)

### Points clés trouvés dans la recherche:
1. ILI9488 sur SPI nécessite **18-bit (RGB666)** obligatoirement
2. Écrans IPS nécessitent **inversion ON** (commande 0x21)
3. VCOM IPS différent: **0x4D au lieu de 0x22**
4. Vitesse max sur breadboard: **4 MHz**

## Licence

Ce projet est fourni "tel quel" pour usage éducatif et de test.

## Auteur

Créé en décembre 2024 pour tester un écran ILI9488 IPS avec ESP32-S3.
Configuration validée et fonctionnelle avec ESP-IDF v5.5.1.
