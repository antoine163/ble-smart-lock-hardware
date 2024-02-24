# Bluetooth LE Smart Lock - Firmware
Il s'agit du firmware de la carte BleSmartLock.


# Commen compiler
1) Installer le complicateur croisé *arm-none-eabi-gcc*.
2) Installer l'outie *cmake*.
2) Télécharger le SDK [STSW-BLUENRG1-DK](https://www.st.com/en/embedded-software/stsw-bluenrg1-dk.html). Vous devrez vous inscrire sur ST.
3) Extrayez avec *innoextract* le contenu du dossier *library* vers `src/board/device/bluenrg-2`.
Par exemple, si vous avez téléchargé *STSW-BLUENRG1-DK* à proximité de README.md :
```
innoextract BlueNRG-1_2\ DK-3.2.3.0-Setup.exe -I app/Library
cp app/Library/* src/board/device/bluenrg-2/
```
4) Vous pouvez spécifier le répertoire de la chaîne d'outils avec la variable d'environnement *ARMGCC_DIR* si le complicateur croisé n'est pas trouvé.

5) Le model du module BlueNRG doit être passé à cmake via la variable *MODEL_BLUENRG* avec comme valeur soit *M2SA* ou *M2SP*. Par example pour compiler avec le BLUENRG-M2SA:
```
export ARMGCC_DIR="/to/directory/of/arm-gcc"
mkdir build
cd build
cmake --toolchain cmake/arm-none-eabi-gcc.cmake -DMODEL_BLUENRG=M2SA -DCMAKE_BUILD_TYPE=Release ..
```

