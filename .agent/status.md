# Status — esphome-projects
> MàJ : 2026-07-13

**État :** Fleet de devices ESPHome (un YAML par device à la racine, packages mutualisés). En cours : test du limiteur de courant soft sur `timothee-bed-light` (bande WS2812 qui fait brown-out l'ESP32 à pleine luminosité).

**Prochaines étapes :**
- [ ] Valider en réel le throttling via PR esphome#15348 (`power_estimation` / `max_current_ma: 8000`) sur `timothee-bed-light`.
- [ ] Si concluant : remplacer `github://pr#15348` par une release ESPHome une fois la PR mergée, et propager aux autres bandes (`parent-bed-light`, `smart-bed-parent`, `daynight-light`…).
- [ ] Limite connue de la PR : throttling inactif pendant les effets → surveiller si crash persiste en mode effet.
