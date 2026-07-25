# Status — esphome-projects
> MàJ : 2026-07-25

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
([AntorFr/esphome-smart-cover](https://github.com/AntorFr/esphome-smart-cover), `ref: main`
en attendant un tag), qui apprend les durées réelles sur les courses butée-à-butée. Les
durées des substitutions ne sont plus qu'un **amorçage**.

**Prochaines étapes (volet) :**
- [ ] Valider l'apprentissage : une course pleine par sens doit produire `Learned open/close duration` dans le log, et survivre à un reboot.
- [ ] Vérifier la justesse d'un `cover.set_position` intermédiaire (40 %).
- [ ] Épingler `ref: v0.1.0` une fois `smart_cover` validé sur le terrain.
- [ ] Généraliser aux autres volets : `packages/shelly-plus-2-pm*.yaml` sont prévus pour, seules les valeurs mesurées changent d'un volet à l'autre.
- [ ] Déclarer le `cover.current_based` et supprimer le bloc calibration du device.
- [ ] Vérifier côté HA que le vrai `pool-filtering-relay` a bien récupéré son nom.
- [ ] Créer le repo `esphome-sun-cover` (composant `sun_cover` : géométrie solaire, modes, arbitrage bouton).

---

**État :** Fleet de devices ESPHome (un YAML par device à la racine, packages mutualisés). En cours : limiteur de courant soft sur `timothee-bed-light` (bande **WS2811 5V** à boules qui fait brown-out l'ESP32). Diagnostic posé : crash à ~0,8 A (palier 56% blanc OK / 57% crash) → l'ESP reset et ouvre le relais `power_supply`. À 0,8 A sur une alim 10A, c'est un **défaut du chemin d'alim** (fil/connecteur PSU→carte), pas un problème de budget. `max_current_ma: 700` posé comme pansement (throttle physique, HA reste à 100%).

**Prochaines étapes :**
- [ ] **Vrai fix : câble/connecteur d'alim PSU→carte** (gros/court) → doit restaurer le blanc plein (~3,6 A).
- [ ] Condo ~1000 µF sur la rail 5V de l'ESP (amont du relais, PAS sur la bande — en aval l'inrush aggrave le brown-out).
- [ ] Tester le retrait de `power_supply: relay` (supprime l'inrush de fermeture).
- [ ] Limite connue de la PR esphome#15348 : throttling inactif pendant les effets → le cap 700 ne protège que le statique.
- [ ] Une fois la PR mergée : remplacer `github://pr#15348` par une release ESPHome, propager aux autres bandes (`parent-bed-light`, `smart-bed-parent`, `daynight-light`…).
