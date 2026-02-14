# Configuration WiFi - ESP32-S3

## ✅ WiFi ajouté avec succès !

Le code WiFi a été intégré au projet. Voici ce qui a été ajouté :

## 📝 Modifications apportées

### 1. [main/main.c](main/main.c)
**Ajouts** :
- Headers WiFi : `esp_wifi.h`, `esp_event.h`, `nvs_flash.h`
- Configuration WiFi (lignes 20-30)
- Gestionnaire d'événements WiFi (`wifi_event_handler`)
- Fonction d'initialisation WiFi (`wifi_init_sta`)
- Initialisation NVS + WiFi dans `app_main`

### 2. [sdkconfig.defaults](sdkconfig.defaults)
**Ajouts** :
- Configuration WiFi
- Configuration LWIP (pile TCP/IP)

### 3. Nouveaux fichiers créés
- **[CREDENTIALS_TEMPLATE.md](CREDENTIALS_TEMPLATE.md)** : Guide complet pour configurer WiFi et obtenir la clé API Google Cloud TTS
- **[WIFI_SETUP.md](WIFI_SETUP.md)** : Ce fichier (instructions WiFi)

## 🔧 Configuration requise

### **AVANT DE COMPILER**, modifiez vos credentials WiFi dans [main/main.c](main/main.c) :

```c
// WiFi Configuration - MODIFIEZ VOS CREDENTIALS ICI
#define WIFI_SSID      "VotreNomWiFi"        // ← Changez ici
#define WIFI_PASS      "VotreMotDePasse"     // ← Changez ici
#define WIFI_MAXIMUM_RETRY  5
```

## 🚀 Compilation et test

### 1. Modifiez vos credentials WiFi (voir ci-dessus)

### 2. Compilez et flashez :

```bash
idf.py build
idf.py -p COM4 flash monitor
```

### 3. Vérifiez les logs

Vous devriez voir :

```
I (xxx) S3_DISPLAY: ESP32-S3 + ILI9488 IPS + LVGL + Audio + WiFi Test
I (xxx) S3_DISPLAY: Initializing WiFi...
I (xxx) S3_DISPLAY: WiFi initialization finished.
I (xxx) S3_DISPLAY: Got IP:192.168.x.x
I (xxx) S3_DISPLAY: Connected to AP SSID: VotreNomWiFi
I (xxx) S3_DISPLAY: Initializing SPI bus...
I (xxx) S3_DISPLAY: Initializing ILI9488 IPS...
```

## 🔍 Dépannage WiFi

### Erreur "Failed to connect to SSID"
- Vérifiez que le SSID est correct (sensible à la casse)
- Vérifiez que le mot de passe est correct
- Assurez-vous que le WiFi est en 2.4 GHz (ESP32 ne supporte pas 5 GHz)

### Retry messages
```
I (xxx) S3_DISPLAY: Retry to connect to the AP (1/5)
```
- Normal si le WiFi met du temps à se connecter
- Si ça échoue 5 fois, vérifiez vos credentials

### "UNEXPECTED EVENT"
- Problème de configuration, vérifiez le code

## 📊 Ordre d'initialisation dans app_main

```
1. NVS Flash init       ← Requis pour stocker config WiFi
2. WiFi init            ← Connexion au réseau
3. GPIO config          ← Pins pour LCD
4. SPI init             ← Bus SPI pour LCD
5. ILI9488 init         ← Initialisation écran
6. LVGL init            ← Bibliothèque graphique
7. Create UI            ← "Hello World"
8. I2S init             ← Audio MAX98357A
9. Test tone            ← 5 bips sonores
10. LVGL loop           ← Boucle principale
```

## 🎯 Prochaine étape : Google Cloud TTS

Une fois le WiFi fonctionnel, consultez [CREDENTIALS_TEMPLATE.md](CREDENTIALS_TEMPLATE.md) pour :
1. Obtenir une clé API Google Cloud Text-to-Speech
2. Configurer l'intégration TTS
3. Faire parler "Hello World" !

## 📚 Fonctionnalités WiFi disponibles

Avec WiFi connecté, vous pouvez maintenant :
- ✅ Faire des requêtes HTTP/HTTPS
- ✅ Utiliser Google Cloud TTS
- ✅ Synchroniser l'heure (NTP)
- ✅ Télécharger des fichiers
- ✅ Communication IoT (MQTT, WebSocket, etc.)

## 🔒 Sécurité

⚠️ **Important** : Ne commitez jamais vos credentials WiFi ou clés API dans un dépôt public !

Si vous utilisez Git :
```bash
# Ajoutez au .gitignore
echo "main/main.c" >> .gitignore
```

Ou créez un fichier séparé pour les credentials (meilleure pratique).
