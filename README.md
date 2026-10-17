# EndfieldLauncher

Tiny C++20 launcher for Arknights: Endfield (Visual Studio 2022).

Double click -> UAC prompt -> game starts. A Hypergryph-signed file from the game folder is
copied to `%TEMP%\EndfieldLauncher\ParentBridge\Games.exe`, started suspended, and used as the parent of `Endfield.exe`.

## Use
Copy `EndfieldLauncher.exe` into the game folder (next to `Endfield.exe`) and run it, or:

    EndfieldLauncher.exe --game "D:\Games\Endfield\Endfield.exe"

| Option | Default |
|---|---|
| `--game <path>` | search `--game-dir` for `Endfield.exe`, then the remembered path, then the official launcher's folders, then ask |
| `--pick` | choose `Endfield.exe` in a dialog and remember it |
| `--install` | create `Endfield Launcher.lnk` on the desktop (replacing any existing one) and exit |
| `--game-dir <dir>` | folder of the launcher exe |
| `--decoy-dir <dir>` | `%TEMP%\EndfieldLauncher\ParentBridge` |
| `--debug` | off (writes `%TEMP%\EndfieldLauncher\launcher.log`, warns if the game exits within 15 s) |
| `--log <file>` | off unless `--debug` |
| `--parent <path>` | installed official `Games.exe`, else decoy copy |
| `--args "<text>"` / `-- <args>` | none |

## Install
The first time the game is found and about to start from a given folder, a Yes/No box asks whether to create the
desktop shortcut. If the desktop already has `Endfield Launcher.lnk` pointing at this exe, nothing is asked. If it has
one that points somewhere else, the box asks whether to overwrite it. The question is asked once per exe path: the path
is remembered (`ShortcutPromptedFor` under `HKCU\Software\EndfieldLauncher`), so the same location is never asked
twice, while moving the exe to another folder asks once more. Delete the value to be asked again.

To create the shortcut without any question, run `EndfieldLauncher.exe --install` from where the exe will stay. It
writes `Endfield Launcher.lnk` to the current user's desktop, replacing a shortcut with that name. The shortcut points
at the exe you ran, uses its icon, and carries `--game "<path>"` if you passed `--game` together with `--install`.

## Build
Open `EndfieldLauncher.sln`, select `Release | x64`, build. Output: `Build\x64\Release\EndfieldLauncher.exe`
(static CRT, `requireAdministrator` manifest: UAC prompt on launch).

See [Building](Documentation/Building.md) for configurations, platforms, the folder layout and the
command-line build.

## License
GNU General Public License v3.0, see [LICENSE](LICENSE).
