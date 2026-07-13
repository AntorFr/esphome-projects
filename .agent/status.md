# Status — esphome-projects
> MàJ : 2026-07-13

**État :** Fleet de devices ESPHome (un YAML par device à la racine, packages mutualisés). En cours : limiteur de courant soft sur `timothee-bed-light` (bande **WS2811 5V** à boules qui fait brown-out l'ESP32). Diagnostic posé : crash à ~0,8 A (palier 56% blanc OK / 57% crash) → l'ESP reset et ouvre le relais `power_supply`. À 0,8 A sur une alim 10A, c'est un **défaut du chemin d'alim** (fil/connecteur PSU→carte), pas un problème de budget. `max_current_ma: 700` posé comme pansement (throttle physique, HA reste à 100%).

**Prochaines étapes :**
- [ ] **Vrai fix : câble/connecteur d'alim PSU→carte** (gros/court) → doit restaurer le blanc plein (~3,6 A).
- [ ] Condo ~1000 µF sur la rail 5V de l'ESP (amont du relais, PAS sur la bande — en aval l'inrush aggrave le brown-out).
- [ ] Tester le retrait de `power_supply: relay` (supprime l'inrush de fermeture).
- [ ] Limite connue de la PR esphome#15348 : throttling inactif pendant les effets → le cap 700 ne protège que le statique.
- [ ] Une fois la PR mergée : remplacer `github://pr#15348` par une release ESPHome, propager aux autres bandes (`parent-bed-light`, `smart-bed-parent`, `daynight-light`…).
