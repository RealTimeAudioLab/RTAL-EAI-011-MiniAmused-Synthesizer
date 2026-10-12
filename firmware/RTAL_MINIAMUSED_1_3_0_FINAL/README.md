# RTAL MiniAmused v1.3.0 FINAL

**Status:** Releasekandidat auf Basis der vom Anwender bestätigten Hardware-Version v1.3.0 J. Gegenüber J sind keine Klang-, Sequenzer-, MIDI- oder UI-Funktionen geändert; ausschließlich Versionsstrings, ein veralteter Kommentar und Release-Dateien wurden angepasst. Ein neuer Hardwaretest der FINAL-Kennung und ein vollständiger Arduino-Build konnten hier nicht durchgeführt werden.

## Neu seit v1.2.7

- Monophoner 16-Step-Sequenzer mit Variationen A/B und Wiedergabe A, B, AB, AABB.
- Individuelle Sequenzlängen (1 bis 16); pro Step Note, Velocity, Gate, REST, TIE, SLIDE, ACCENT.
- Tempo, Rate, Swing, MIDI-Clock-Synchronisierung, Keyboard-Transposition, Variation COPY A→B.
- Touch-Oberfläche mit 8 × 2 Steps, Step-Detailansicht, STEP REC und LIVE REC.
- Erweiterte Preset-Speicherung (V3), einschließlich beider Variationen; alte V1/V2-Dateien sind mit dem neuen Gerät ladbar.
- Verbesserte Statusanzeige im Preset-Browser: `SAVING...`, `SAVE OK` und `READY`; Fehler bleiben sichtbar.
- Überarbeitete Step-Matrix, ARP/SEQ-Tabs, geglättete externe BPM-Anzeige und Startbild v1.3.0.

## Hardware / Toolchain

- ESP32-S3 / Guition JC4827W543, Touch-TFT 480×272, GT911.
- Bewährtes Projekt-Setup verwenden: Arduino IDE 2.3.10, ESP32 Core 3.3.11, LVGL 8.4.0.
- `.ino` und Ordner heißen beide exakt `RTAL_MINIAMUSED_1_3_0_FINAL`.

## Sicherheit der SD-Bank

Vor der ersten V3-Speicherung vollständige Sicherung von `/RTAL_MINIAMUSED/presets/` anlegen. Die neue Firmware liest alte V1/V2-Presets, die ältere v1.2.7-Firmware kann jedoch die neuen V3-Dateien nicht wieder einlesen. Vorhandene Presets deshalb nicht ohne Backup überschreiben.

## Übernahme / abschließender Kurztest

1. Sketch mit denselben Board-/LVGL-Einstellungen wie v1.3.0 J kompilieren und flashen.
2. Startbild und serielle Kennung `v1.3.0 FINAL` prüfen.
3. ARP-/SEQ-Bedienung, interne und externe Clock testen.
4. Beide Sequenzvariationen verändern, Preset speichern, erneut laden und vergleichen.
5. Ein vorhandenes V2-Preset laden; danach mit Backup ein neues V3-Preset anlegen.
6. `SAVE OK` nach Wartezeit auf `READY` sowie erneute Bestätigung bei zweitem SAVE kontrollieren.

## Bezug zur Hardwarefreigabe

Der Nutzer hat v1.3.0 J mit „funktioniert“ bestätigt. Da der Programmcode für Audio, Engine, MIDI, Presets und UI gegenüber J unverändert übernommen wurde, ist FINAL eine Release-Umbenennung dieses bestätigten Entwicklungsstandes, nicht eine neue Funktionsversion.
