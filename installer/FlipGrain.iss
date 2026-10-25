; FlipGrain VST3 Installer (Inno Setup 6)
; Build:  ISCC.exe /DAppVersion=1.0.0 /DPluginDir="C:\...\FlipGrain.vst3" installer\FlipGrain.iss

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef PluginDir
  #define PluginDir "..\build\FlipGrain_artefacts\Release\VST3\FlipGrain.vst3"
#endif
#define AppName      "FlipGrain"
#define AppPublisher "YourCompany"
#define AppURL       "https://example.com"
#define Bundle       "FlipGrain.vst3"

[Setup]
AppId={{B6F1D6A4-3C1E-4D0E-9C1B-7F3A2E5D8A11}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
VersionInfoVersion={#AppVersion}
DefaultDirName={commoncf64}\VST3\{#Bundle}
UsePreviousAppDir=yes
DisableProgramGroupPage=yes
DirExistsWarning=no
AppendDefaultDirName=no
OutputDir=Output
OutputBaseFilename=FlipGrain-{#AppVersion}-Windows-Setup
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
WizardImageFile=assets\wizard_large.bmp,assets\wizard_large_150.bmp,assets\wizard_large_200.bmp
WizardSmallImageFile=assets\wizard_small.bmp,assets\wizard_small_150.bmp,assets\wizard_small_200.bmp
LicenseFile=EULA.txt
UninstallDisplayName={#AppName} VST3 {#AppVersion}
CloseApplications=no

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl"

[Files]
Source: "{#PluginDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{app}"

[Messages]
en.WelcomeLabel1=Welcome to FlipGrain
en.WelcomeLabel2=Drop in any sample. Chop it. Flip it into a better melody.%n%nThis wizard installs the FlipGrain VST3 plug-in on your computer.%n%nPlease close your DAW before you continue.
en.FinishedHeadingLabel=Ready to flip
en.FinishedLabel=FlipGrain has been installed.%n%nRescan your plug-ins in your DAW and look for FlipGrain under Instruments / Generators.
de.WelcomeLabel1=Willkommen bei FlipGrain
de.WelcomeLabel2=Sample reinziehen, choppen, zu einer besseren Melodie flippen.%n%nDieser Assistent installiert das FlipGrain VST3-Plugin.%n%nBitte schliesse vorher deine DAW.
de.FinishedHeadingLabel=Bereit zum Flippen
de.FinishedLabel=FlipGrain wurde installiert.%n%nScanne in deiner DAW die Plugins neu und suche FlipGrain unter Instrumenten / Generatoren.

[CustomMessages]
en.QuickCaption=Quick start
en.QuickDesc=Your first flip in 30 seconds
en.QuickSub=Read this once - it is worth it.
en.QuickBody=1.  Load FlipGrain as an instrument (FL Studio: Add > FlipGrain).%n2.  Drag an audio file onto the waveform - or copy a file in Explorer and press Ctrl+V.%n3.  Play the chops from your piano roll, starting at the root note (default C2 = MIDI 36).%n4.  Hit FLIP for a new melody. Drag the MIDI pad into your piano roll to keep the pattern.%n%nTip: FL Studio custom plug-in folder? Pick it on the previous page.
de.QuickCaption=Schnellstart
de.QuickDesc=Dein erster Flip in 30 Sekunden
de.QuickSub=Einmal lesen lohnt sich.
de.QuickBody=1.  FlipGrain als Instrument laden (FL Studio: Add > FlipGrain).%n2.  Audiodatei auf die Waveform ziehen - oder im Explorer kopieren und Strg+V druecken.%n3.  Chops ueber die Piano Roll spielen, ab der Root-Note (Standard C2 = MIDI 36).%n4.  FLIP klicken fuer eine neue Melodie. MIDI-Pad in die Piano Roll ziehen, um das Pattern zu behalten.%n%nTipp: Eigener Plugin-Ordner in FL Studio? Waehle ihn auf der vorigen Seite.

[Code]
procedure InitializeWizard;
begin
  CreateOutputMsgMemoPage(wpSelectDir,
    ExpandConstant('{cm:QuickCaption}'),
    ExpandConstant('{cm:QuickDesc}'),
    ExpandConstant('{cm:QuickSub}'),
    ExpandConstant('{cm:QuickBody}'));
  WizardForm.DirEdit.Hint := 'VST3 folder';
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := False;
end;
