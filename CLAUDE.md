# Recommandations projet ESP-32 : Screen, sound and mic

## Sécurité
- Ne JAMAIS publier de clé API, mot de passe, SSID ou toute autre donnée sensible dans les commits. Toujours utiliser des placeholders (ex: "VOTRE_CLE_API", "VOTRE_MOT_DE_PASSE").
- Vérifier systématiquement l'absence de credentials avant chaque commit.

## Workflow Git
- Ne JAMAIS faire de commit ou de push sans l'autorisation explicite de l'utilisateur.
- Il est possible de proposer un commit, mais toujours attendre la validation avant de l'exécuter.
- Quand l'utilisateur demande un commit, faire automatiquement le push vers GitHub dans la foulée.
