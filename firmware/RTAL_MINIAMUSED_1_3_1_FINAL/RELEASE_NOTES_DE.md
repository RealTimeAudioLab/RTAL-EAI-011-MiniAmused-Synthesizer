# Release Notes – RTAL MiniAmused v1.3.1 FINAL

RTAL MiniAmused v1.3.1 führt einen integrierten zweifachen 16-Step-Sequenzer mit Touch-Editor, MIDI STEP/LIVE Recording, Variationsmodi, Swing und MIDI-Clock-Synchronisierung ein. Sequenzen A/B werden zusammen mit dem Synthesizer-Klang in Presets im V3-Format gespeichert. Frühere V1/V2-Presets bleiben unter v1.3.1 ladbar.

Der Release bewahrt die bewährte Synthesizer-Architektur aus v1.2.7 und enthält Verbesserungen der Performance-GUI, stabilere BPM-Anzeige sowie korrigierte Preset-Statusmeldungen. Der integrierte Splash-Screen trägt v1.3.1.

**Achtung:** Preset-Dateien vor dem Überschreiben sichern; ältere v1.2.7-Firmware kann V3-Presets nicht laden.

## V1.3.1 FINAL – NV3041A Hardware-Reset

- GPIO5 als Open-Drain-Ausgang: 50 ms LOW, anschließend 150 ms Freigabe vor `sGfx->begin()`.
- Erfordert Hardwareumbau: GPIO5 über 220 Ω an RSTI (Knoten R10/C35); R25 entfernt; bestehende RC-Schaltung bleibt erhalten.
- Serielle Diagnose mit Präfix `[LCD RESET]`.
- Audio, MIDI, Sequencer, Presets, Touch und Display-Flush unverändert.
- Achtung: Flashgröße und Partitionsschema müssen zur realen Platine passen (getestete SYNTECH-Platine: 4 MB).
- **Status:** Quellcodeänderung erstellt; Build und Hardwaretest dieser MiniAmused-Version stehen noch aus.
