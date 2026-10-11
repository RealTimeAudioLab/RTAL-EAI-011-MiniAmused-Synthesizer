# RTAL MiniAmused – 127 zusätzliche/Factory-Presets

**Version:** Für RTAL MiniAmused v1.2.7 FINAL und kompatible v1.2.x-Firmware mit dem Preset-V2-Format.

Dieses Paket enthält **genau 127 spielbare Preset-Dateien**, Slots **000 bis 126**. Slot **127 bleibt frei** für eigene Sounds. Die Firmware unterstützt insgesamt 128 Slots (000–127).

## Was ist enthalten?

- 16 BASS (000–015), 16 LEAD (016–031), 16 PAD (032–047), 16 PLUCK (048–063)
- 16 BRASS/WIND (064–079), 16 KEYS/ORGAN (080–095), 16 ARP/SEQ (096–111)
- 8 TEXTURE/FX (112–119), 7 PERC/SPECIAL (120–126)
- `PRESET_KATALOG.md` und `PRESET_KATALOG.csv` als Übersicht
- `INSTALL_NUR_FEHLENDE_PRESETS.bat` zum **nicht überschreibenden Kopieren unter Windows**

## SD-Karten-Verzeichnis

```text
SD-KARTE:/
└── RTAL_MINIAMUSED/
    └── presets/
        ├── 000.rtal
        ├── 001.rtal
        ├── ...
        └── 126.rtal
```

**Wichtig: Bestehende eigene Presets nicht überschreiben!**

### Empfohlen: Automatisch nur freie Slots befüllen (Windows)

1. ZIP auf dem Computer **vollständig entpacken**; das Windows-Skript **nicht innerhalb des ZIP** starten.
2. SD-Karte in den Kartenleser stecken und den Laufwerksbuchstaben feststellen (z.B. `E:`).
3. `INSTALL_NUR_FEHLENDE_PRESETS.bat` durch Doppelklick starten; den Laufwerksbuchstaben eingeben.
4. Das Skript kopiert nur `000.rtal` ... `126.rtal`, die auf der SD-Karte **noch nicht existieren**.
5. SD-Karte sicher entfernen; im MiniAmused die PRESET-Seite öffnen und einen Slot auswählen, dann **LOAD** drücken.

Der Installer löscht und überschreibt **keine bestehenden Presets**. Bestehende Presets behalten daher ihren Klang und Namen; die resultierende Bank kann in einzelnen Slots von der mitgelieferten Factory-Liste abweichen. Der Installer zählt die Dateien, prüft aber nicht deren musikalische Funktion.

### Manuelle Installation

Den Inhalt des enthaltenen Ordners `RTAL_MINIAMUSED` in das Stammverzeichnis der SD-Karte kopieren. **Beim Windows-Konfliktdialog »Dateien am Ziel überspringen« wählen**, damit vorhandene Dateien erhalten bleiben.

### Eine vollständig identische Factory-Bank gewünscht?

Vorher unbedingt die vorhandenen `.rtal`-Dateien sichern. Nur wenn du ausdrücklich alle eigenen Sounds ersetzen willst, kannst du nach dem Backup die vorhandenen Slots 000–126 durch die mitgelieferten ersetzen. Ohne Backup gibt es keine Möglichkeit, die alten Klänge aus der Factory-Bank wiederherzustellen.

## Bedienung und Hinweise

- **PRESET UP/DOWN wählt nur den Slot; das Preset wird erst mit LOAD übernommen.**
- Beim Speichern im MiniAmused werden die Parameter im gleichen V2-Format erzeugt.
- Die ARP-Presets 096–111 schalten den Arpeggiator ein. Dafür MIDI-Noten oder einen Akkord spielen; `HOLD` bleibt aus und die interne Clock ist aktiv (es ist kein MIDI Clock Input nötig).
- `DUAL` bedeutet maximal zwei unabhängige Oszillator-Stimmen, die den nachfolgenden Filter und die Hüllkurven teilen.
- Starke Effekte und Resonanzen am Anfang mit **reduzierter Abhörlautstärke** testen.
- Einige Geräusch-Presets verwenden ausschließlich den internen Rauschgenerator. Trotzdem muss für die Lautstärkehüllkurve eine MIDI-Note gehalten werden.

## Technische Daten / Herkunft

- Die Parameternummerierung wurde direkt aus `ParameterIDs.h` der v1.2.7 FINAL abgeleitet (69 Parameter, IDs 0–68).
- Die Datei entspricht exakt der von `PresetManager.cpp` verwendeten V2-Struktur: Header `RTAL_MINIMOOG_PRESET_V2` (historischer Formatname), `name`, `param_count=69`, alle 69 normierten Werte und `complete=1`.
- Discrete Auswahlwerte für Wellenform, Oszillatorlage, ARP-Modus und Delay-Ping-Pong-Modus sind auf die Quantisierung der Firmware abgestimmt.
- Sämtliche Dateien wurden auf Vollständigkeit, numerische Wertebereiche und verschiedene Parametersätze geprüft.
- Die Soundprogramme sind kreative Ausgangspunkte. Sie sind **nicht individuell auf der ESP32-S3-Hardware klanglich getestet**; Feineinstellungen nach Gehör können sinnvoll sein.
- Die Firmware wird durch dieses Paket nicht verändert. Die Verbesserungen der Filter-CUTOFF-Berechnung aus v1.2.7 sind bei den Presets wirksam.
