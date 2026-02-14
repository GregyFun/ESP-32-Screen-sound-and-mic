# Guide de Câblage - ESP32-S3 + ILI9488 IPS Display

## Vue d'ensemble de votre écran

**Écran**: ILI9488 480x320 IPS TFT LCD avec touch FT6236 et lecteur SD
**Connexion**: SPI pour LCD/SD, I2C pour touch
**Nombre de pins**: 14
**IMPORTANT**: Cet écran est un modèle **IPS** - il nécessite un code d'initialisation spécifique pour IPS (pas pour TN)

## Pinout de l'écran (de gauche à droite)

| # | Pin Écran | Fonction | Type | ESP32-S3 GPIO | Notes |
|---|-----------|----------|------|---------------|-------|
| 1 | **SD_CS** | SD Card Chip Select | SPI | Non connecté | Optionnel - pour carte SD uniquement |
| 2 | **CTP_INT** | Touch Interrupt | I2C | Non connecté | Optionnel - pour touch uniquement |
| 3 | **CTP_SDA** | Touch I2C Data | I2C | Non connecté | Optionnel - pour touch uniquement |
| 4 | **CTP_RST** | Touch Reset | GPIO | Non connecté | Optionnel - pour touch uniquement |
| 5 | **CTP_SCL** | Touch I2C Clock | I2C | Non connecté | Optionnel - pour touch uniquement |
| 6 | **SDO** | SPI Data Out (MISO) | SPI | Non connecté | Optionnel - lecture uniquement |
| 7 | **LED** | Backlight Control | GPIO | Non connecté | Optionnel - l'écran s'allume sans |
| 8 | **SCK** | SPI Clock | SPI | **GPIO 12** | **OBLIGATOIRE** |
| 9 | **SDI** | SPI Data In (MOSI) | SPI | **GPIO 11** | **OBLIGATOIRE** |
| 10 | **LCD_RS** | Data/Command (DC) | SPI | **GPIO 9** | **OBLIGATOIRE** |
| 11 | **LCD_RST** | LCD Reset | GPIO | **GPIO 14** | **OBLIGATOIRE** |
| 12 | **LCD_CS** | LCD Chip Select | SPI | **GPIO 10** | **OBLIGATOIRE** |
| 13 | **GND** | Ground | Power | GND | **OBLIGATOIRE** |
| 14 | **VCC** | Power 3.3V | Power | 3.3V | **OBLIGATOIRE** |

## Configuration GPIO confirmée (fonctionne)

Pour afficher sur l'écran LCD **ILI9488 IPS**, seuls ces 7 câbles sont nécessaires:

```c
// LCD SPI Pins - CONFIGURATION TESTÉE ET VALIDÉE
#define PIN_NUM_MOSI   11    // SDI (Pin 9 de l'écran)
#define PIN_NUM_CLK    12    // SCK (Pin 8 de l'écran)
#define PIN_NUM_CS     10    // LCD_CS (Pin 12 de l'écran)
#define PIN_NUM_DC     9     // LCD_RS/Data-Command (Pin 10 de l'écran)
#define PIN_NUM_RST    14    // LCD_RST (Pin 11 de l'écran)
// + VCC (Pin 14) vers 3.3V
// + GND (Pin 13) vers GND

// MISO non connecté - pas nécessaire pour l'affichage
// Backlight non connecté - l'écran s'allume automatiquement
```

## GPIO à éviter sur ESP32-S3

**ÉVITEZ ces GPIO** car ils ont des fonctions spéciales:
- **GPIO 0**: Boot mode (utilisé au démarrage)
- **GPIO 19, 20**: USB (si vous utilisez USB pour programmer)
- **GPIO 26-32**: Connectés à la flash SPI (NE PAS UTILISER)
- **GPIO 33-37**: Réservés pour PSRAM Octal-SPI (NE PAS UTILISER sur votre modèle avec 8MB PSRAM)

## Procédure de câblage (minimal pour LCD)

### Câbles OBLIGATOIRES (7 fils):

1. **GND** (Pin 13) → **GND** de l'ESP32-S3
2. **VCC** (Pin 14) → **3.3V** de l'ESP32-S3
3. **SDI/MOSI** (Pin 9) → **GPIO 11**
4. **SCK** (Pin 8) → **GPIO 12**
5. **LCD_CS** (Pin 12) → **GPIO 10**
6. **LCD_RS/DC** (Pin 10) → **GPIO 9**
7. **LCD_RST** (Pin 11) → **GPIO 14**

### Câbles OPTIONNELS (non nécessaires pour commencer):

- **SDO/MISO** (Pin 6): Pour lire depuis l'écran (ID, pixels) - non utilisé
- **LED/Backlight** (Pin 7): Contrôle du rétroéclairage - l'écran s'allume automatiquement
- **Touch pins** (Pins 2-5): Pour utiliser l'écran tactile
- **SD_CS** (Pin 1): Pour utiliser le lecteur de carte SD

## Points critiques ILI9488 IPS

### 1. Format de couleur 18-bit (RGB666)
**CRITIQUE**: L'ILI9488 sur interface SPI nécessite **18 bits par pixel** (3 octets RGB666), **PAS 16 bits** (RGB565).

```c
// Pixel Format Set - 18bit (obligatoire!)
lcd_cmd(0x3A);
uint8_t pixfmt[] = {0x66};  // 0x66 = 18 bits/pixel
lcd_data(pixfmt, sizeof(pixfmt));
```

### 2. Inversion d'affichage pour IPS
**CRITIQUE**: Les écrans IPS nécessitent l'inversion ON, contrairement aux écrans TN.

```c
// Display Inversion ON (critique pour IPS!)
lcd_cmd(0x21);
```

### 3. VCOM spécifique IPS
Les écrans IPS nécessitent des paramètres VCOM différents:

```c
// VCOM Control (IPS specific: -0.79688V)
lcd_cmd(0xC5);
uint8_t vcom[] = {0x00, 0x4D, 0x80};  // Pas 0x22, mais 0x4D!
lcd_data(vcom, sizeof(vcom));
```

### 4. Soft Reset obligatoire
Toujours commencer par un reset logiciel:

```c
// Software reset (DOIT être la première commande)
lcd_cmd(0x01);
vTaskDelay(pdMS_TO_TICKS(100));
```

## Vitesse SPI

- **4 MHz maximum** recommandé avec câbles Dupont/breadboard
- L'ILI9488 supporte jusqu'à 80 MHz, mais les câbles non blindés limitent la vitesse pratique
- Utiliser **SPI Mode 0** (CPOL=0, CPHA=0)
- Mode **Half-duplex** (transmission unidirectionnelle vers l'écran)

## Après le câblage

1. **Vérifiez vos connexions** (surtout VCC et GND!)
2. **Compilez et flashez** le projet
3. **Vérifiez le moniteur série** - vous devriez voir:
   ```
   I (xxx) S3_DISPLAY: ESP32-S3 + ILI9488 Display Test
   I (xxx) S3_DISPLAY: Initializing SPI bus...
   I (xxx) S3_DISPLAY: Initializing ILI9488 IPS...
   I (xxx) S3_DISPLAY: ILI9488 IPS initialized
   I (xxx) S3_DISPLAY: Filling screen with RED...
   ```
4. **L'écran doit afficher**: Rouge → Vert → Bleu en boucle

## Dépannage

### L'écran reste blanc:
- ✅ **RÉSOLU**: Utilisez le format 18-bit (RGB666) au lieu de 16-bit
- ✅ **RÉSOLU**: Activez l'inversion d'affichage (0x21) pour IPS
- ✅ **RÉSOLU**: Ajustez le VCOM à 0x4D pour IPS
- Vérifiez l'alimentation (VCC et GND)
- Vérifiez les connexions SPI

### Lecture ID retourne 0xFF:
- **Normal** - MISO non connecté, la lecture ne fonctionne pas
- Pas nécessaire pour l'affichage

### L'écran affiche des couleurs incorrectes:
- Vérifiez que vous utilisez bien le format RGB666 (3 octets/pixel)
- Vérifiez que l'inversion est activée (commande 0x21)

## Câblage Module Audio MAX98357A (pour TTS)

### Vue d'ensemble
**Module**: MAX98357A I2S Class D Amplifier
**Connexion**: I2S (3 pins + alimentation)
**Haut-parleur**: 4Ω ou 8Ω, 3W recommandé

### Pinout MAX98357A → ESP32-S3

| MAX98357A Pin | ESP32-S3 GPIO | Fonction | Notes |
|---------------|---------------|----------|-------|
| **VIN** | 5V ou 3.3V | Alimentation | Préférer 5V pour plus de volume |
| **GND** | GND | Masse | **OBLIGATOIRE** |
| **DIN** | **GPIO 16** | I2S Data | **OBLIGATOIRE** (testé sur ESP32-S3) |
| **BCLK** | **GPIO 21** | I2S Bit Clock | **OBLIGATOIRE** (testé sur ESP32-S3) |
| **LRC** | **GPIO 17** | I2S Word Select | **OBLIGATOIRE** (testé sur ESP32-S3) |
| **SD** | **3.3V** | Shutdown | **Connecter à 3.3V** (mode actif) |
| **GAIN** | Non connecté | Gain control | Non connecté = 9dB (par défaut) |

### Connexion Haut-parleur

```
Haut-parleur (4Ω ou 8Ω):
  Fil + (rouge)  →  MAX98357A pin +
  Fil - (noir)   →  MAX98357A pin -
```

### Configuration GPIO utilisées (récapitulatif)

```c
// Écran ILI9488 (SPI)
#define PIN_NUM_MOSI   11    // SDI
#define PIN_NUM_CLK    12    // SCK
#define PIN_NUM_CS     10    // LCD_CS
#define PIN_NUM_DC     9     // LCD_RS
#define PIN_NUM_RST    14    // LCD_RST

// Audio MAX98357A (I2S) - Configuration validée ESP32-S3
#define I2S_BCLK       21    // Bit Clock (testé sur ESP32-S3)
#define I2S_LRC        17    // Word Select (testé sur ESP32-S3)
#define I2S_DIN        16    // Data (testé sur ESP32-S3)
```

### Notes sur l'audio

- **Volume**: Le MAX98357A peut être bruyant! Commencez avec un haut-parleur de faible puissance (0.5W-1W) pour tester
- **Alimentation**: Si le volume est faible, alimentez VIN avec 5V au lieu de 3.3V
- **Gain**: Pour ajuster le volume, connectez GAIN à GND (3dB), VIN (15dB), ou laissez flottant (9dB)
- **Qualité audio**: Excellent pour la synthèse vocale (Text-to-Speech)

## Notes importantes

- Le bus SPI est **partagé** entre LCD et SD Card (différents CS)
- Le Touch utilise un bus **I2C séparé**
- Le bus **I2S** est dédié à l'audio (MAX98357A)
- **SDO (MISO)** n'est pas nécessaire pour l'affichage
- Le **backlight (LED)** fonctionne automatiquement (alimenté en interne)
- **IPS vs TN**: Ne jamais utiliser du code TN sur un écran IPS, les couleurs seront inversées

## Ressources

- [Datasheet ILI9488](https://www.displayfuture.com/Display/datasheet/controller/ILI9488.pdf)
- [ESP-IDF SPI Master Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/spi_master.html)
- [GitHub Discussion - ILI9488 IPS on ESP32](https://github.com/Bodmer/TFT_eSPI/discussions/3115)
