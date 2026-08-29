FROM: user (Karol)
TO: claude + kiro
DATE: 2026-06-15T14:00:00+02:00
STATUS: OVER

---

Test fizyczny firmware v0.3.6 + APK v10.

## Co zaobserwowano (chronologicznie)

1. Polaczenie nie nastapilo w <2s - trwalo **okolo minute**.
2. W trakcie tej minuty: **WiFi telefonu sie zawiesilo, wyskoczyl wykrzyknik** "brak internetu" przy sieci Flower.
3. Wykrzyknik **nie znikal** nawet potem.
4. App wyrzucila komunikat "nie udalo sie zaladowac githuba".
5. **Ekran czytnika sie odswiezyl** (sam, jakby restart UI).
6. App znowu zaczela szukac czytnika, ale **niczego nie znalazla** (bo wykrzyknik przy WiFi = HyperOS blokuje sieciowy dostep procesu).
7. Stan koncowy: nic.

## Dodatkowe objawy po stronie czytnika

- **Po ~15 sekundach na ekranie Sync, ekran sie zacina** - nie mozna kliknac niczego.
- Odswiezenie ekranu (jakies dzialanie) sprawia ze odpowiada.
- Powtarzalne.

## Co nie zadzialalo z aplikacja

- Log w zakladce Debug zaczyna sie **od momentu polaczenia z czytnikiem**, nie od otwarcia aplikacji - czyli nie widac co app robi w trakcie tych 60 sekund probowania.
