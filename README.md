# FlipGrain – Sample Flipper & Granular Melody Engine (VST3)

Sample reinziehen (Drag & Drop oder Ctrl+V), automatisch choppen, in die Tonart retunen, mit **FLIP** zu neuen Melodien umbauen und die Chops per Piano Roll spielen.

## Features
- Transienten-Slicing (Spectral Flux) oder 4/8/16/32 Raster, YIN-Pitch pro Chop, automatische Key-Erkennung
- **Chop-Mode**: jede Taste ab Root-Note (Standard 36) = ein Chop, live auf Skala gerastet (Retune)
- **FLIP**: neue Skalenstufen, Oktaven, Reverse und Tastenzuordnung pro Chop (reproduzierbar per Seed, wird im Projekt gespeichert)
- **Keys-Mode**: ein Chop (oder das ganze Sample) chromatisch spielbar
- Granular-Engine (Position, Scan, Size, Density, Spray, Jitter, Spread, Reverse, Shape) oder Classic-Sampler
- Drag-MIDI-Pad: erzeugt ein 2-Takte-Pattern aus den Chops und zieht es in deine DAW
- Sample wird FLAC-komprimiert im Projekt gespeichert (portabel)

## Bauen (GitHub)
1. Repo anlegen, Inhalt pushen (`main`).
2. Tab **Actions** -> "Build Windows VST3 + Installer" läuft automatisch.
3. Unter *Artifacts* `FlipGrain-Windows` herunterladen: `FlipGrain-x.y.z-Windows-Setup.exe` + portable ZIP.
4. Release: `git tag v1.0.0 && git push --tags` -> Installer hängt automatisch am GitHub Release.

## Vor dem Verkauf anpassen
- `CMakeLists.txt`: `COMPANY_NAME`, `PLUGIN_MANUFACTURER_CODE`, `PLUGIN_CODE` (eindeutig!)
- `installer/FlipGrain.iss`: Publisher, URL, **AppId neu generieren**; `installer/EULA.txt` rechtlich prüfen lassen
- **JUCE-Lizenz**: Closed-Source-Verkauf braucht eine kommerzielle JUCE-Lizenz (sonst AGPL/GPL-Pflichten, JUCE-Splash)
- **VST3**: Steinberg-Lizenzvereinbarung/Logo-Regeln beachten ("VST is a trademark of Steinberg Media Technologies GmbH")
- Installer **code-signieren** (sonst SmartScreen-Warnung) – z. B. Azure Trusted Signing oder ein OV/EV-Zertifikat
- Name "FlipGrain" auf Markenkollisionen prüfen

## Installationsort
`C:\Program Files\Common Files\VST3\FlipGrain.vst3` (im Installer änderbar, z. B. für eigene FL-Studio-Ordner).

## Hinweis
Dieser Code wurde ohne Compiler-Lauf geschrieben. Falls der erste CI-Build Fehler meldet: Log kopieren und mir schicken, dann fixe ich sie.
