# Guide Text-to-Speech (TTS) - Google Cloud

## ✅ État actuel de l'implémentation

### Fonctionnalités implémentées :

- ✅ Client HTTPS pour Google Cloud TTS API
- ✅ Génération de requêtes JSON
- ✅ Réception de la réponse (audio en base64)
- ✅ Décodage base64 → MP3 binaire
- ⏳ **Décodage MP3 → PCM** (À finaliser avec bibliothèque MP3)
- ⏳ Lecture audio via I2S

### Ce qui fonctionne maintenant :

Le code actuel appelle avec succès l'API Google Cloud TTS et récupère l'audio MP3 encodé en base64.

**Logs attendus** :
```
I (xxx) S3_DISPLAY: Attempting Google Cloud TTS...
I (xxx) S3_DISPLAY: === Starting TTS: Hello World ===
I (xxx) S3_DISPLAY: Calling Google Cloud TTS API for: "Bonjour le monde"
I (xxx) S3_DISPLAY: HTTP_EVENT_ON_CONNECTED
I (xxx) S3_DISPLAY: HTTP Status = 200
I (xxx) S3_DISPLAY: Response received: XXXX bytes
I (xxx) S3_DISPLAY: Got audio content (base64 length: XXXX)
I (xxx) S3_DISPLAY: Base64 decoded: XXXX bytes of MP3 data
W (xxx) S3_DISPLAY: MP3 decoding not yet implemented - this is a placeholder
```

## 🔧 Configuration requise

### 1. Obtenir une clé API Google Cloud TTS

Suivez le guide dans [CREDENTIALS_TEMPLATE.md](CREDENTIALS_TEMPLATE.md) pour obtenir votre clé API.

### 2. Ajouter la clé dans credentials.h

Ouvrez [main/credentials.h](main/credentials.h) et ajoutez votre clé :

```c
#define GOOGLE_TTS_API_KEY "AIzaSy_VOTRE_CLE_ICI"
```

### 3. Compiler et tester

```bash
# Full clean + build (recommandé après ajout de dépendances)
idf.py fullclean
idf.py build
idf.py -p COM4 flash monitor
```

## 📝 Comment ça fonctionne

### Séquence d'exécution :

1. **WiFi** se connecte au réseau
2. **Écran** affiche "Hello World"
3. **Audio test** joue 5 bips (pour vérifier le hardware)
4. **TTS** appelle l'API Google Cloud :
   - Envoie : `{"input":{"text":"Bonjour le monde"}...}`
   - Reçoit : `{"audioContent":"UklGRi4...."}`  (MP3 en base64)
   - Décode le base64 → données MP3 brutes
   - ⏳ **TODO** : Décode MP3 → PCM 16kHz
   - ⏳ **TODO** : Joue via I2S

## 🎯 Prochaine étape : Décodeur MP3

Pour entendre réellement la voix, il faut ajouter un décodeur MP3.

### Option A : Bibliothèque minimp3 (recommandé)

**Avantages** :
- Légère (~10KB)
- Pas de dépendances
- Simple à intégrer

**Fichier à créer** : `main/minimp3_decode.c`

### Option B : ESP-ADF (ESP Audio Development Framework)

**Avantages** :
- Décodeur MP3 complet
- Support de nombreux codecs

**Inconvénient** :
- Plus lourd (~500KB)

### Option C : Demander LINEAR16 au lieu de MP3

Modifiez dans [main/main.c](main/main.c:523) :

```c
// Au lieu de:
cJSON_AddStringToObject(audioConfig, "audioEncoding", "MP3");

// Utilisez:
cJSON_AddStringToObject(audioConfig, "audioEncoding", "LINEAR16");
```

**Avantage** : Pas besoin de décodeur MP3 ! L'audio est déjà en PCM.

**Inconvénient** : Fichiers ~10x plus gros (mais pour "Bonjour le monde", ça reste petit).

## 🧪 Test sans clé API

Si vous n'avez pas encore de clé API, le code vérifie et affiche :

```
E (xxx) S3_DISPLAY: Google TTS API Key is empty! Please add it to credentials.h
E (xxx) S3_DISPLAY: TTS API call failed
```

C'est normal ! Ajoutez votre clé API pour tester.

## 🔊 Paramètres TTS configurables

Dans [main/main.c](main/main.c:518-520), vous pouvez modifier :

### Langue et voix :

```c
// Français (femme)
cJSON_AddStringToObject(voice, "languageCode", "fr-FR");
cJSON_AddStringToObject(voice, "name", "fr-FR-Standard-A");

// Anglais (homme)
cJSON_AddStringToObject(voice, "languageCode", "en-US");
cJSON_AddStringToObject(voice, "name", "en-US-Standard-B");

// Liste complète des voix :
// https://cloud.google.com/text-to-speech/docs/voices
```

### Format audio :

```c
// MP3 (nécessite décodeur)
cJSON_AddStringToObject(audioConfig, "audioEncoding", "MP3");

// LINEAR16 (PCM direct, pas de décodage)
cJSON_AddStringToObject(audioConfig, "audioEncoding", "LINEAR16");
```

### Fréquence d'échantillonnage :

```c
// Doit correspondre à SAMPLE_RATE (16000 Hz)
cJSON_AddNumberToObject(audioConfig, "sampleRateHertz", SAMPLE_RATE);
```

## 📊 Utilisation mémoire

```
Heap libre avant TTS : ~250KB
Heap utilisé par TTS  : ~70KB (buffer HTTP + décodage base64)
Heap après TTS        : ~250KB (mémoire libérée)
```

Le code libère automatiquement toute la mémoire allouée.

## 🐛 Dépannage

### "Google TTS API Key is empty!"

Ajoutez votre clé dans `main/credentials.h` :
```c
#define GOOGLE_TTS_API_KEY "AIza..."
```

### "HTTP Status = 400"

Erreur dans la requête JSON. Vérifiez les logs pour voir la requête envoyée.

### "HTTP Status = 401" ou "HTTP Status = 403"

Clé API invalide ou restrictions d'API mal configurées.
- Vérifiez que l'API Cloud Text-to-Speech est activée
- Vérifiez que votre clé API est correcte
- Vérifiez les restrictions de clé (IP, API, etc.)

### "Failed to parse JSON response"

La réponse de Google n'est pas au format attendu. Activez les logs debug :

```c
// Dans main.c, changez le niveau de log
esp_log_level_set(TAG, ESP_LOG_DEBUG);
```

## 🎵 Prochaines améliorations possibles

1. ✅ **Intégrer décodeur MP3** (minimp3 ou ESP-ADF)
2. **Cache TTS** : Sauvegarder "Hello World" en flash pour éviter les appels API répétés
3. **TTS dynamique** : Lire du texte variable (heure, température, etc.)
4. **Contrôle tactile** : Appuyer sur l'écran pour déclencher la voix
5. **Multi-langue** : Basculer entre français/anglais

## 📚 Ressources

- [Google Cloud TTS Documentation](https://cloud.google.com/text-to-speech/docs)
- [Liste des voix disponibles](https://cloud.google.com/text-to-speech/docs/voices)
- [API Reference](https://cloud.google.com/text-to-speech/docs/reference/rest/v1/text/synthesize)
- [Tarification](https://cloud.google.com/text-to-speech/pricing)
