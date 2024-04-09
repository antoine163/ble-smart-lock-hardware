# Bluetooth LE Smart Lock
Ce project est une serrure électronique à déverrouillage par Bluetooth Low Energie.  

Il est constitué de 4 parties:
- D'une carte électronique basée sur le module [bluenrg-m2](https://www.st.com/en/wireless-connectivity/bluenrg-m2.html).
- D'un firmware pour le module [bluenrg-m2](https://www.st.com/en/wireless-connectivity/bluenrg-m2.html).
- D'une carte électronique secondaire déportent le capteur de luminosité. (optionnelle).
- D'une application Androïde pour le pilotage de la serrure.

## Fonctions principales
- Déverrouillage via Bluetooth Low Energie.
- Ouverture par bouton poussoir.
- Verrouillage automatique par perte de connection BLE.

## Fonctions secondaire
- Éclairage d'ambience RGBW, pour l'affiche de l’état de la serrure.
- Éclairage extérieur la nuit quand la serrure est ouverte.


### États de la serrure

| BLE           | Serrure | Éclairage ambience  | Éclairage extérieur   | Voyant Bouton |
| ------------- | :-----: | :------:            | :------:              | :----:        |
| Non connecté  | Fermé   | Éteint              | Éteint                | Éteint        |
| Connecté      | Fermé   | Vert                | Éteint                | Éteint        |
| Déverrouillé  | Fermé   | Bleu                | Éteint                | **Allumé**    | 
||                              
| Non connecté  | Ouvert  | Rouge               | Éteint                | Éteint        |
| Connecté      | Ouvert  | **Blanc**           | **Allumé**            | Éteint        |
| Déverrouillé  | Ouvert  | **Blanc**           | **Allumé**            | Éteint        |


# Fonctionnalités

## Firmware de la serrure
- Connection radio via Bluetooth LE.
- Contrôle de l'ouverture de la serrure.
- Bouton poussoir lumineux extérieure pour ouverture de la serrure et affichage de la possibilité d'ouverture.
- Detection de la luminosité ambient.
- Pilotage de ruban led RGBW 12V, pour affiche de l'état de la serrure.
- Pilotage d'un éclairage 12V ([Mini-Light](https://github.com/antoine163/Mini-Light/tree/master/Elec/light))

## Application Android
- Scan et appairage avec la serrure BLE.
- Renommage du la serrure BLE.
- Déverrouillage et réglage de la proximité BLE.
- Ouverture par bouton de la serrure BLE.
- Notification lors de l'éloignement avec la serrure no verrouiller."




