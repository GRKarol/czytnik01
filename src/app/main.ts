import "./app.element";
import { registerSW } from "virtual:pwa-register";

registerSW({
  immediate: true,
  onNeedRefresh() {
    /* aktualizacja gotowa - SW przejmie po reloadzie */
  },
  onOfflineReady() {
    /* aplikacja gotowa do pracy offline */
  },
});
