# Guide de Test TTS - Prêt à Tester!

## ✅ État: Implémentation Complète

Le système TTS est maintenant **entièrement implémenté** et prêt pour les tests.

## 📋 Ce qui a été fait

### Code implémenté:
- ✅ Client HTTPS pour Google Cloud TTS API
- ✅ Génération de requêtes JSON avec cJSON
- ✅ Format audio LINEAR16 (PCM direct, pas de décodeur MP3 nécessaire)
- ✅ Décodage base64 → PCM binaire
- ✅ Lecture audio via I2S vers MAX98357A
- ✅ Séparation des credentials dans [main/credentials.h](main/credentials.h)
- ✅ Protection des credentials via .gitignore

### Configuration matérielle:
- ✅ MAX98357A connecté sur GPIO 21/17/16
- ✅ WiFi configuré et testé (vous avez confirmé "le wifi est ok")
- ✅ Test audio 5 bips fonctionnel

## 🚀 Comment Tester

### Étape 1: Vérifier vos credentials

Ouvrez [main/credentials.h](main/credentials.h) et assurez-vous que:

```c
// WiFi Configuration (ESP32-S3 = 2.4 GHz UNIQUEMENT!)
#define WIFI_SSID      "VOTRE_SSID"  // ← Votre SSID WiFi 2.4 GHz
#define WIFI_PASS      "VOTRE_MOT_DE_PASSE"  // ← Votre mot de passe

// Google Cloud TTS API Key
#define GOOGLE_TTS_API_KEY "VOTRE_CLE_API"  // ← Votre clé API
```

⚠️ **IMPORTANT**: Vérifiez que votre clé API Google Cloud TTS est valide:
- Elle doit commencer par `AIza`
- L'API "Cloud Text-to-Speech" doit être activée dans votre projet Google Cloud
- Voir [CREDENTIALS_TEMPLATE.md](CREDENTIALS_TEMPLATE.md) si vous n'avez pas encore de clé

### Étape 2: Compiler le projet

```bash
# Nettoyage complet (recommandé après changements de dépendances)
idf.py fullclean

# Compilation
idf.py build
```

**Résultat attendu**: Compilation réussie sans erreurs CMake.

### Étape 3: Flasher et monitorer

```bash
# Flasher et voir les logs
idf.py -p COM4 flash monitor
```

### Étape 4: Observer la séquence complète

Vous devriez voir cette séquence:

#### 1. Initialisation (0-5 secondes)
```
I (xxx) S3_DISPLAY: Initializing display...
I (xxx) S3_DISPLAY: Display initialized successfully
I (xxx) S3_DISPLAY: Initializing WiFi...
I (xxx) S3_DISPLAY: WiFi initialization finished.
```

#### 2. Connexion WiFi (5-10 secondes)
```
I (xxx) S3_DISPLAY: Got IP:192.168.x.x
I (xxx) S3_DISPLAY: Connected to AP SSID: VOTRE_SSID
```

#### 3. Affichage graphique (10 secondes)
```
I (xxx) S3_DISPLAY: Creating label...
I (xxx) S3_DISPLAY: Label created successfully
```
→ L'écran affiche "Hello World!" en blanc sur fond noir

#### 4. Initialisation Audio I2S (12 secondes)
```
I (xxx) S3_DISPLAY: Initializing I2S...
I (xxx) S3_DISPLAY: I2S initialized successfully
```

#### 5. Test Audio - 5 Bips (12-15 secondes)
```
I (xxx) S3_DISPLAY: Playing test tone at 880 Hz...
I (xxx) S3_DISPLAY: Playing beep 1/5
I (xxx) S3_DISPLAY: Playing beep 2/5
I (xxx) S3_DISPLAY: Playing beep 3/5
I (xxx) S3_DISPLAY: Playing beep 4/5
I (xxx) S3_DISPLAY: Playing beep 5/5
I (xxx) S3_DISPLAY: Test tone finished
```
→ Vous devez **entendre 5 bips courts** via le MAX98357A

#### 6. Appel TTS Google Cloud (17-20 secondes)
```
I (xxx) S3_DISPLAY: Attempting Google Cloud TTS...
I (xxx) S3_DISPLAY: === Starting TTS: Bonjour le monde ===
I (xxx) S3_DISPLAY: Calling Google Cloud TTS API for: "Bonjour le monde"
```

#### 7. Connexion HTTPS
```
I (xxx) S3_DISPLAY: HTTP_EVENT_ON_CONNECTED
I (xxx) S3_DISPLAY: HTTP Status = 200
```

#### 8. Réception et traitement audio
```
I (xxx) S3_DISPLAY: Response received: XXXXX bytes
I (xxx) S3_DISPLAY: Got audio content (base64 length: XXXXX)
I (xxx) S3_DISPLAY: Base64 decoded: XXXXX bytes of LINEAR16 PCM audio
I (xxx) S3_DISPLAY: Playing LINEAR16 audio...
```

#### 9. Lecture audio (20-23 secondes)
```
I (xxx) S3_DISPLAY: Audio playback complete! (wrote XXXXX bytes)
I (xxx) S3_DISPLAY: === TTS completed successfully ===
```
→ Vous devez **entendre "Bonjour le monde"** en français (voix féminine)

## ✅ Test Réussi Si:

1. ✅ L'écran affiche "Hello World!"
2. ✅ Vous entendez 5 bips (test audio hardware)
3. ✅ HTTP Status = 200 (API Google répond)
4. ✅ Vous entendez "Bonjour le monde" en français

## ❌ Dépannage des erreurs possibles

### Erreur: "Google TTS API Key is empty!"

```
E (xxx) S3_DISPLAY: Google TTS API Key is empty! Please add it to credentials.h
E (xxx) S3_DISPLAY: TTS API call failed
```

**Solution**: Ajoutez votre clé API dans [main/credentials.h](main/credentials.h):
```c
#define GOOGLE_TTS_API_KEY "AIzaSy_VOTRE_VRAIE_CLE_ICI"
```

Voir [CREDENTIALS_TEMPLATE.md](CREDENTIALS_TEMPLATE.md) pour obtenir une clé.

### Erreur: "HTTP Status = 400"

```
E (xxx) S3_DISPLAY: HTTP Status = 400
E (xxx) S3_DISPLAY: TTS synthesis failed
```

**Cause**: Erreur dans la requête JSON (normalement impossible avec le code actuel)

**Solution**: Vérifiez les logs pour voir la requête envoyée. Signalez-moi l'erreur complète.

### Erreur: "HTTP Status = 401" ou "HTTP Status = 403"

```
E (xxx) S3_DISPLAY: HTTP Status = 401
```

**Cause**: Clé API invalide ou restrictions d'API mal configurées

**Solutions**:
1. Vérifiez que la clé API est correcte (commence par `AIza`)
2. Allez sur [Google Cloud Console](https://console.cloud.google.com/)
3. Vérifiez que l'API "Cloud Text-to-Speech" est **activée**
4. Vérifiez les restrictions de clé (IP, APIs autorisées, etc.)

### Erreur: WiFi ne se connecte pas

```
E (xxx) S3_DISPLAY: WiFi connection failed
```

**Solutions**:
- Vérifiez que vous utilisez un réseau **2.4 GHz** (ESP32-S3 ne supporte PAS 5 GHz!)
- Vérifiez le SSID et mot de passe dans [main/credentials.h](main/credentials.h)
- Voir [WIFI_SETUP.md](WIFI_SETUP.md) pour plus de détails

### Erreur: Pas de son (mais HTTP Status = 200)

**Vérifications**:
1. Le MAX98357A est-il correctement alimenté (VIN, GND)?
2. Les câbles sont-ils bien connectés (DIN=GPIO16, BCLK=GPIO21, LRC=GPIO17)?
3. Le haut-parleur est-il connecté au MAX98357A?
4. Entendez-vous les 5 bips de test? Si non, c'est un problème hardware
5. Le volume du MAX98357A est-il à 0? (pas de potentiomètre de volume, normalement à fond)

### Erreur: Compilation échoue

```
CMake Error: Component "espressif/esp-adf-libs" not found
```

**Solution**: Ce problème a été corrigé. Vérifiez que [main/idf_component.yml](main/idf_component.yml) contient uniquement:
```yaml
dependencies:
  lvgl/lvgl:
    version: "^8.3.0"
```

Si le problème persiste:
```bash
idf.py fullclean
idf.py build
```

## 🎵 Personnaliser la voix

### Changer la langue

Dans [main/main.c](main/main.c#L518-L520), modifiez:

```c
// Anglais (homme)
cJSON_AddStringToObject(voice, "languageCode", "en-US");
cJSON_AddStringToObject(voice, "name", "en-US-Standard-B");

// Français (homme)
cJSON_AddStringToObject(voice, "languageCode", "fr-FR");
cJSON_AddStringToObject(voice, "name", "fr-FR-Standard-B");

// Liste complète: https://cloud.google.com/text-to-speech/docs/voices
```

### Changer le texte à dire

Dans [main/main.c](main/main.c#L577), modifiez:

```c
// Au lieu de "Bonjour le monde"
char *audio_base64 = google_tts_synthesize("Votre texte ici");
```

## 📊 Utilisation mémoire attendue

```
Heap libre au démarrage: ~250 KB
Heap utilisé par TTS   : ~70 KB (buffer HTTP + base64)
Heap après TTS         : ~250 KB (mémoire libérée automatiquement)
```

## 💰 Coût de l'API

**Quota gratuit Google Cloud TTS**: 4 millions de caractères/mois

"Bonjour le monde" = 17 caractères → Vous pouvez faire **~235 000 tests gratuits par mois** !

## 📚 Fichiers de documentation

- [TTS_GUIDE.md](TTS_GUIDE.md) - Guide technique complet du TTS
- [CREDENTIALS_TEMPLATE.md](CREDENTIALS_TEMPLATE.md) - Guide pour obtenir votre clé API
- [WIRING_GUIDE.md](WIRING_GUIDE.md) - Schéma de câblage complet
- [WIFI_SETUP.md](WIFI_SETUP.md) - Configuration WiFi détaillée

## 🎯 Prochaines étapes possibles

Une fois le TTS fonctionnel, vous pourrez:

1. **Affichage dynamique** : Afficher le texte TTS sur l'écran avant de le dire
2. **Interaction tactile** : Appuyer sur l'écran pour déclencher la voix
3. **TTS variable** : Dire l'heure, la température, etc.
4. **Multi-langue** : Basculer entre français/anglais
5. **Cache TTS** : Sauvegarder l'audio en flash pour éviter les appels API répétés

## 🚨 En cas de problème

Si vous rencontrez une erreur non documentée ici:

1. Copiez les logs complets (depuis le démarrage jusqu'à l'erreur)
2. Notez à quelle étape le problème survient (WiFi, HTTP, audio, etc.)
3. Vérifiez le `HTTP Status` code
4. Vérifiez la taille des données reçues

---

**Prêt à tester?** Lancez `idf.py -p COM4 flash monitor` et observez les logs ! 🎤
