# Instrukcje repo dla Claude

## Remote workflow — twarda zasada

Dwa remote: `staging` (GRKarol/czytnik01-staging, prywatne) i `origin`
(GRKarol/czytnik01, publiczne — hostuje GitHub Pages i release'y).

- Domyślny cel każdego push to `staging`. Cała bieżąca praca, redesign,
  buildy .apk debug idą tam.
- Nigdy nie pushuj na `origin` (żaden branch, żaden release/*) bez jawnego
  potwierdzenia Karola typu "wypchnij na główne repo" / "rób release".
  Zaakceptowanie buildu, "wygląda dobrze", odpowiedź na apkę czy test na
  telefonie to NIE jest zgoda na push na `origin` — to zgoda tylko na to,
  co jest na `staging`.
- Push na `origin/main` uruchamia realny deploy na GitHub Pages, więc błąd
  tutaj jest widoczny publicznie od razu.

Pełny opis w README.md, sekcja "Workflow: staging vs release".
