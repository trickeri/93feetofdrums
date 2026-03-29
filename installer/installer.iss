; VOID Drum Engine - Inno Setup Installer Script
; Requires Inno Setup 6+ (https://jrsoftware.org/isinfo.php)
; Build with: iscc installer.iss

#define MyAppName "VOID Drum Engine"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "93feetofdrums"
#define MyAppURL "https://github.com/trickeri/93feetofdrums"

[Setup]
AppId={{8E3F2A1B-C4D5-4E6F-A7B8-9C0D1E2F3A4B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
DefaultDirName={commonpf}\Common Files\VST3
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=VOID_Drum_Engine_Setup_v{#MyAppVersion}
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
DisableDirPage=no
DirExistsWarning=no
UsePreviousAppDir=yes
UninstallDisplayName={#MyAppName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; VST3 plugin bundle
Source: "..\build\VOIDDrumEngine_artefacts\Release\VST3\VOID Drum Engine.vst3\*"; DestDir: "{app}\VOID Drum Engine.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Dirs]
; Create sample folder structure in AppData
Name: "{userappdata}\VOID Drum Engine\VOID_Samples\kicks"
Name: "{userappdata}\VOID Drum Engine\VOID_Samples\snares"
Name: "{userappdata}\VOID Drum Engine\VOID_Samples\hats"
Name: "{userappdata}\VOID Drum Engine\VOID_Samples\percs"
Name: "{userappdata}\VOID Drum Engine\VOID_Samples\toms"
Name: "{userappdata}\VOID Drum Engine\VOID_Samples\fx"
Name: "{userappdata}\VOID Drum Engine\VOID_Samples\user"
Name: "{userappdata}\VOID Drum Engine\VOID_Presets\Factory"
Name: "{userappdata}\VOID Drum Engine\VOID_Presets\User"

[Messages]
WelcomeLabel2=This will install {#MyAppName} v{#MyAppVersion} VST3 plugin on your computer.%n%nBy default the plugin installs to the standard VST3 directory. You can choose a different location on the next page.%n%nSample folders will be created at:%n  %APPDATA%\VOID Drum Engine\VOID_Samples\

[UninstallDelete]
; Clean up empty sample dirs on uninstall (won't delete user files)
Type: dirifempty; Name: "{userappdata}\VOID Drum Engine\VOID_Samples\kicks"
Type: dirifempty; Name: "{userappdata}\VOID Drum Engine\VOID_Samples\snares"
Type: dirifempty; Name: "{userappdata}\VOID Drum Engine\VOID_Samples\hats"
Type: dirifempty; Name: "{userappdata}\VOID Drum Engine\VOID_Samples\percs"
Type: dirifempty; Name: "{userappdata}\VOID Drum Engine\VOID_Samples\toms"
Type: dirifempty; Name: "{userappdata}\VOID Drum Engine\VOID_Samples\fx"
Type: dirifempty; Name: "{userappdata}\VOID Drum Engine\VOID_Samples\user"
Type: dirifempty; Name: "{userappdata}\VOID Drum Engine\VOID_Samples"
Type: dirifempty; Name: "{userappdata}\VOID Drum Engine\VOID_Presets\Factory"
Type: dirifempty; Name: "{userappdata}\VOID Drum Engine\VOID_Presets\User"
Type: dirifempty; Name: "{userappdata}\VOID Drum Engine\VOID_Presets"
Type: dirifempty; Name: "{userappdata}\VOID Drum Engine"
