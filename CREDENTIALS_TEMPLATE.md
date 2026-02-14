# Configuration des Credentials

## Instructions

### 1. Créer votre fichier credentials.h

Le fichier `main/credentials.h` contient vos credentials WiFi et votre clé API Google Cloud TTS.

**Première utilisation** :
```bash
# Copiez le template vers credentials.h
cp main/credentials.h.template main/credentials.h
```

Puis modifiez `main/credentials.h` avec vos vraies valeurs :

```c
// WiFi Configuration
#define WIFI_SSID      "VotreNomWiFi"        // ← Votre SSID 2.4 GHz
#define WIFI_PASS      "VotreMotDePasse"     // ← Votre mot de passe WiFi
#define WIFI_MAXIMUM_RETRY  5

// Google Cloud TTS API Key
#define GOOGLE_TTS_API_KEY "AIzaSy..."  // ← Votre clé API
```

⚠️ **IMPORTANT** :
- ESP32-S3 supporte **UNIQUEMENT le WiFi 2.4 GHz** (PAS 5 GHz!)
- Utilisez un réseau comme "TP-Link" ou "MonWiFi-2.4G"
- NE PAS utiliser "TP-Link-5G" ou réseaux 5 GHz uniquement

### 2. Sécurité

✅ Le fichier `credentials.h` est **automatiquement ignoré par Git** (via `.gitignore`)

✅ Vous pouvez partager votre code sans exposer vos mots de passe

✅ Le fichier `credentials.h.template` sert de modèle (versionné dans Git)

## Comment obtenir une clé API Google Cloud TTS

### Étape 1 : Créer un projet Google Cloud
1. Allez sur [Google Cloud Console](https://console.cloud.google.com/)
2. Cliquez sur le sélecteur de projet en haut
3. Cliquez sur **"NEW PROJECT"**
4. Donnez un nom à votre projet (ex: "ESP32-TTS")
5. Cliquez sur **"CREATE"**

### Étape 2 : Activer l'API Text-to-Speech
1. Dans le menu à gauche, allez dans **"APIs & Services" → "Library"**
2. Recherchez **"Cloud Text-to-Speech API"**
3. Cliquez sur l'API dans les résultats
4. Cliquez sur **"ENABLE"**

### Étape 3 : Créer une clé API
1. Allez dans **"APIs & Services" → "Credentials"**
2. Cliquez sur **"+ CREATE CREDENTIALS"** en haut
3. Sélectionnez **"API Key"**
4. Une clé sera générée (commence par `AIza...`)
5. **COPIEZ cette clé** (vous ne pourrez pas la voir à nouveau)
6. (Optionnel) Cliquez sur **"RESTRICT KEY"** pour limiter l'accès à Cloud Text-to-Speech API uniquement

### Étape 4 : Ajouter la clé dans credentials.h

Ouvrez `main/credentials.h` et ajoutez votre clé :

```c
#define GOOGLE_TTS_API_KEY "AIzaSyDEMO_KEY_1234567890abcdefg"  // ← Collez votre vraie clé ici
```

### Étape 5 : Sécuriser votre clé (IMPORTANT)

⚠️ **Ne partagez JAMAIS votre clé API publiquement !**

- ✅ Le fichier `credentials.h` est déjà dans `.gitignore`
- ✅ Ne commitez jamais `credentials.h` dans Git
- ✅ Ne partagez pas de screenshots contenant votre clé
- ✅ Utilisez les restrictions de clé sur Google Cloud Console

## Tarification Google Cloud TTS

**Quota gratuit mensuel** : 4 millions de caractères
- Standard voices : Gratuit jusqu'à 4M caractères/mois
- WaveNet voices : 1M caractères gratuits/mois

Pour "Hello World" (11 caractères), vous pourriez faire environ **360 000 requêtes gratuites par mois** !

## Test de connexion WiFi

Une fois vos credentials WiFi configurés dans `main/credentials.h`, compilez et flashez :

```bash
idf.py build
idf.py -p COM4 flash monitor
```

Vous devriez voir dans les logs :
```
I (xxx) S3_DISPLAY: Initializing WiFi...
I (xxx) S3_DISPLAY: WiFi initialization finished.
I (xxx) S3_DISPLAY: Got IP:192.168.x.x
I (xxx) S3_DISPLAY: Connected to AP SSID: VotreNomWiFi
```

## Prochaines étapes

1. ✅ Créez `main/credentials.h` à partir du template
2. ✅ Configurez vos credentials WiFi
3. ✅ Testez la connexion WiFi
4. ⏳ Obtenez votre clé API Google Cloud TTS
5. ⏳ Ajoutez la clé dans `credentials.h`
6. ⏳ Intégration TTS (prochaine étape)

## Structure des fichiers

```
main/
├── credentials.h           ← VOS credentials (NON versionné, ignoré par Git)
├── credentials.h.template  ← Template (versionné dans Git)
└── main.c                  ← Code principal (inclut credentials.h)

.gitignore                  ← Protège credentials.h
CREDENTIALS_TEMPLATE.md     ← Ce fichier (instructions)
```

## Dépannage

### "credentials.h: No such file or directory"

Vous devez créer `main/credentials.h` à partir du template :

```bash
cp main/credentials.h.template main/credentials.h
```

Puis modifiez `main/credentials.h` avec vos vraies valeurs.

### WiFi ne se connecte pas

- Vérifiez que vous utilisez un réseau **2.4 GHz** (pas 5 GHz)
- Vérifiez le SSID (sensible à la casse)
- Vérifiez le mot de passe
- Consultez [WIFI_SETUP.md](WIFI_SETUP.md) pour plus de dépannage
