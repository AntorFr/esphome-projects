# Status — esphome-projects
> MàJ : 2026-08-11

**salon-ble-relay : carte Olimex ESP32-POE-ISO morte, remplacée (2026-08-11).** Autopsie :
offline depuis le matin, USB énumère (CH340 OK) mais ESP32 muet jusque dans sa ROM (esptool
sans réponse malgré l'auto-reset natif) → module HS. Carte neuve (ESP32-D0WD-V3 rev 3.1,
MAC `d8:bc:38:f7:9a:88`) flashée en série, YAML inchangé. Dette optionnelle relevée par le
boot log : `minimum_chip_revision: "3.1"` + `sram1_as_iram: true` (+40 Ko d'IRAM, utile à
un proxy BLE) — à poser lors d'un prochain OTA.

**Linky : barème Tempo du 1ᵉʳ août 2026 flashé en OTA (fait le 2026-08-11)** — le
compteur valorise au nouveau barème (`dff16b0`, six substitutions).

**esp-heishamon en mode actif (2026-08-10)** : l'ESP (carte HeishaMon ESP32) est seul maître
sur le CN-CNT — `listen_only: false` + `tx_enable_pin: GPIO5`. Détails et suivi dans le
`status.md` du repo `esphome-heishamon`.

---

**Volet roulant cuisine / Shelly Plus 2PM (nouveau chantier) :** archi arrêtée — hardware
dans `packages/shelly-plus-2-pm.yaml`, mouvement en YAML natif (`cover.current_based`),
intelligence solaire dans un composant externe dédié `sun_cover` (repo à créer).
`kitchen-shutter.yaml` est en **étape 1 / calibration** : flashé en OTA le 2026-07-25
(ESPHome 2026.7.2), device nommé et câblé pour de bon, mais les relais restent exposés
bruts le temps de mesurer le mapping relais/entrées, les durées de course et les seuils
de courant. Il tournait jusqu'ici sous le firmware `pool-filtering-relay` (config Shelly
**Pro** : SPI sur la bobine du relais 1) et squattait son nom DNS — nom désormais libéré.

**Matériel confirmé par les logs (plus d'hypothèse) :** ESP32 rev3.1 **2 cœurs**, PCB
**v0.1.9** (ADE7953 trouvé en 0x38 sur SDA GPIO26 / SCL GPIO25, IRQ GPIO27). Les
substitutions par défaut du package sont donc les bonnes, aucune surcharge de broche.
Métrologie vivante : 234,5 V, ~0,018 A de plancher de bruit sur les deux canaux.

**Mapping mesuré le 2026-07-25 (plus d'hypothèse) :** bouton montée → entrée 1 (GPIO5)
→ relais 1 (GPIO13) → **canal A** ; bouton descente → entrée 2 (GPIO18) → relais 2
(GPIO12) → **canal B**. Câblage « naturel », aucune inversion. Moteur : **0,45 A / ~100 W**
en régime contre **0,018 A** de plancher de bruit → `moving_current_threshold: 0.2` tombe
au milieu. Les **deux** canaux comptent la puissance en négatif (`power_a/b_sign: -1`).
Le **STOP arme bien les deux entrées** simultanément (hypothèse centrale du composant,
validée) ; les appuis directionnels sont séparés de ~1 s, la signature est franche.
Impulsions de 230 à 600 ms.

⚠️ Les horodatages des logs API sont ceux de **réception client** et le WiFi du Shelly
livre en rafales (économie d'énergie) : inutilisables pour mesurer un écart fin entre
deux fronts. Mesurer l'écart des 2 entrées **on-device** (`millis()`) avant de figer la
fenêtre de détection du STOP.

**Calibration terminée le 2026-07-25.** Le courant **chute au plancher de bruit** en
butée (le moteur a ses propres fins de course internes qui coupent l'alimentation — ce
n'est pas un calage), ce qui est le cas nominal de `current_based`. Durées lues dans la
chute de courant, pas dans le temps d'alimentation du relais : **montée 25,7 s**,
**descente 21,3 s** (la montée est 20 % plus lente, le moteur soulève le tablier).
`packages/shelly-plus-2-pm-cover.yaml` écrit et validé : cover `current_based`, relais
passés en `internal`, décodage du bouton mural en YAML (provisoire, à céder au composant).

**Cover en service depuis le 2026-07-25.** Position en %, butées par chute de courant,
bouton mural (montée / descente / STOP simultané) fonctionnels. Interpolation vérifiée à
3,9 %/s contre 3,89 %/s théoriques.

⚠️ **Piège `current_based` résolu — à connaître avant de réutiliser ce package.** Le
contrôle `malfunction_detection` teste `courant_du_circuit_opposé > moving_threshold`
**à chaque `loop()`, sans aucun délai de grâce** (contrairement à la détection de butée,
gardée par `start_sensing_delay`). Toute inversion directe de sens le déclenche donc en
faux positif, car l'ADE7953 moyenne sur 1 s et rapporte encore le courant moteur : le
volet s'arrête au lieu de s'inverser, depuis le bouton **comme depuis HA**. Corrigé par
les capteurs `open_current` / `close_current` qui masquent un canal pendant
`metering_settle_ms` après l'ouverture de son propre relais. **Le masque ne suffit pas
seul** : `current_based` lit la dernière valeur *publiée*, donc le zéro doit être publié
**synchronement** dans le `on_turn_off` du relais (l'action de direction s'exécute dans la
même pile d'appels, avant le `loop()` suivant). Un relais réellement soudé reste détecté,
avec 2 s de retard.

**Écart réel entre les 2 contacts du STOP : 4 ms** (mesuré on-device). `stop_detect_window`
est à 120 ms, valeur de prudence posée avant mesure — 50 ms suffisent largement et
diviseraient par deux la latence du bouton mural.

Le device consomme le composant externe **`smart_cover`**
([AntorFr/esphome-smart-cover](https://github.com/AntorFr/esphome-smart-cover), épinglé
`ref: v0.1.0`), qui apprend les durées réelles sur les courses butée-à-butée et les
maintient à jour. **Les durées des substitutions ne sont plus qu'un amorçage** : les
valeurs qui font foi sont apprises et persistées en NVS (montée 26,4 s, descente 22,0 s
au 2026-07-25). Validation détaillée dans le `status.md` du repo du composant.

**Suivi solaire déployé (v0.2.0, non éprouvé).** Baie **est à 85°** (axe de la maison
174,9°/354,9° calculé depuis deux points GPS de la façade ; la vitre regarde à l'est de
cet axe), **200 cm** vitrés au sol, `closed_position: 23%` (lame finale posée, ajours
encore ouverts). `max_penetration: 50 cm` en valeur de départ, à caler sur l'ombre réelle.
Heure par **SNTP**, pas par HA : la géométrie se calcule ici et ne doit pas dépendre de HA.
L'interrupteur « Suivi solaire » démarre à **OFF** ; c'est une automatisation HA (prévisions
météo) qui décidera quand l'armer. Tout mouvement manuel le coupe ; un **appui long sur
STOP** le rallume.

**Volet salon 1 / second Shelly Plus 2PM (2026-08-11) :** `livingroom-shutter-1.yaml` créé
(étape 1 / calibration, relais bruts) et **flashé en série** (ESPHome 2026.7.4, désormais en
venv `.venv/` local au repo). Remplacera `cover.salon_volet_roulant_1` (nommé `-1` car
l'entité HA l'était — salon multi-volets présumé). Puce sondée par esptool avant flash :
ESP32-U4WDH **rev 3.1 dual-core** — jumelle de la cuisine, donc PCB v0.1.9 attendu (aucun
warning i2c/ade7953 au boot). Boot vérifié, WiFi joint, mDNS OK (config_hash du build).
☠️ **Gotcha durable** (documenté dans l'en-tête de `packages/shelly-plus-2-pm.yaml`) : la
flash embarquée du U4WDH vit sur des pads SPI définis en eFuse (CLK:6 Q:17 D:8 HD:11 CS:16)
→ le stub esptool ne la voit pas (« Failed to communicate with the flash chip », mort au
premier bloc quelle que soit la vitesse) — **`--no-stub` obligatoire**. Matériel de flash :
VoltLink, EN+IO0 câblés = auto-reset esptool, zéro manipulation.

**Deux Shelly 2PM Gen3 en staging (2026-08-11) :** `shutter-1.yaml` / `shutter-2.yaml`
(noms génériques volontaires — pièces non affectées, à renommer à la pose) + nouveau
package `packages/shelly-2pm-gen3.yaml` : ESP32-C3 8 Mo, même ADE7953 que le Plus 2PM,
**mêmes ids d'entités** → le package cover s'empilera tel quel. Les deux flashées en
série (stub OK sur C3 — le `--no-stub` est propre au U4WDH), sondées 8 Mo avant flash
(des C38F 4 Mo existent → boot loop si on déclare 8, cf. en-tête du package), vérifiées
en ligne : entités complètes via `/events`, ADE7953 répondant. MACs : shutter-1 =
`28:37:2f:30:12:e0`, shutter-2 = `e4:b0:63:e8:f5:64`.

**Volet salon 1 EN SERVICE (2026-08-11, posé au mur + calibré + cover validé).** Mesures
sur l'unité : relais 1/canal A = montée 28,1 s, relais 2/canal B = descente 25,0 s,
régime 0,60 A / ~137 W (plancher 0,018 A) → seuils 0,2 / **1,0** A (moteur plus gourmand
que la cuisine, obstacle relevé en proportion), signes puissance −1/−1. ⚠️ **Boutons
muraux CROISÉS vs la cuisine** (montée → entrée 2, descente → entrée 1) → le package
cover est désormais **paramétrable** (`input1_script`/`input2_script`, défauts inchangés).
Test fonctionnel OTA : close 24,9 s → CLOSED, open 27,1 s → OPEN, butées par chute de
courant. Piège rejoué au passage : l'API REST du web_server 2026 route par le **nom
d'entité** (`/switch/Relais 1/turn_on`, encodé), pas par un slug, et exige un
`Content-Length` sur les POST (sinon 411 silencieux).

**Salon 1 : suivi solaire déployé (2026-08-11, non éprouvé).** `smart_cover` v0.2.0 + sun.
Baie **ouest à 265°** (même axe de façade que la cuisine 174,9°/354,9°, côté opposé →
soleil d'après-midi/soirée), **200 cm** vitrés au sol (mesuré — l'amorçage cuisine tombait
juste), **`closed_position: 12%`** (mesuré : volet posé à la main lame au sol / fentes
ouvertes, position relue dans le device — zone de compression bien plus courte que les
23 % de la cuisine). `max_penetration: 50` à caler sur l'ombre réelle un après-midi
ensoleillé. « Suivi solaire » démarre OFF (automatisation HA à créer, comme la cuisine) ;
appui long STOP le réarme. Boutons muraux validés à l'usage après correction du bug de
contre-vérification croisée (commit `2a4920d`).

**Prochaines étapes (volet) :**
- [ ] Salon 1 : côté HA, brancher les consommateurs de l'ancien `cover.salon_volet_roulant_1`
      sur le nouveau cover (et retirer l'ancien module).
- [ ] Salon 1 : un après-midi ensoleillé, mesurer le tapis de lumière et caler
      `max_penetration` (50 cm posé en valeur de départ).
- [ ] Salon 1 : automatisation HA (prévisions météo) pour armer « Suivi solaire ».
- [x] Vérifier la justesse d'un `cover.set_position` intermédiaire (40 %) — validé.
- [ ] **2026-07-26 au matin** : mesurer la profondeur du tapis de lumière volet ouvert, à une heure notée, pour recaler le modèle et régler `max_penetration`.
- [ ] Ajouter une coupure thermique au package (le `shelly-pro-2-pm.yaml` en a une à 90 °C, celui-ci n'en a pas — la sonde monte à 54 °C après une série de courses).
- [ ] Généraliser aux autres volets : `packages/shelly-plus-2-pm*.yaml` sont prévus pour, seules les valeurs mesurées changent d'un volet à l'autre.
- [ ] Quand `smart_cover` v0.2 sortira (arbitrage des commandes), retirer le décodage bouton provisoire de `packages/shelly-plus-2-pm-cover.yaml`.
- [ ] Déclarer le `cover.current_based` et supprimer le bloc calibration du device.
- [ ] Vérifier côté HA que le vrai `pool-filtering-relay` a bien récupéré son nom.
- [ ] Créer le repo `esphome-sun-cover` (composant `sun_cover` : géométrie solaire, modes, arbitrage bouton).

---

**État :** Fleet de devices ESPHome (un YAML par device à la racine, packages mutualisés). En cours : limiteur de courant soft sur `timothee-bed-light` (bande **WS2811 5V** à boules qui fait brown-out l'ESP32). Diagnostic posé : crash à ~0,8 A (palier 56% blanc OK / 57% crash) → l'ESP reset et ouvre le relais `power_supply`. À 0,8 A sur une alim 10A, c'est un **défaut du chemin d'alim** (fil/connecteur PSU→carte), pas un problème de budget. `max_current_ma: 700` posé comme pansement (throttle physique, HA reste à 100%).

**Prochaines étapes :**
- [x] **Linky : flasher** le nouveau barème en OTA — fait le 2026-08-11.
- [ ] Dette repérée au passage : le capteur « Prix kWh Rouge HP » porte l'id `HPJV` et vit
      isolé en bas de `linky.yaml` — à renommer `HPJR` dans un commit à part.
- [ ] **Vrai fix : câble/connecteur d'alim PSU→carte** (gros/court) → doit restaurer le blanc plein (~3,6 A).
- [ ] Condo ~1000 µF sur la rail 5V de l'ESP (amont du relais, PAS sur la bande — en aval l'inrush aggrave le brown-out).
- [ ] Tester le retrait de `power_supply: relay` (supprime l'inrush de fermeture).
- [ ] Limite connue de la PR esphome#15348 : throttling inactif pendant les effets → le cap 700 ne protège que le statique.
- [ ] Une fois la PR mergée : remplacer `github://pr#15348` par une release ESPHome, propager aux autres bandes (`parent-bed-light`, `smart-bed-parent`, `daynight-light`…).
