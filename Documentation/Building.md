# Building EndfieldLauncher

## Requirements

- Visual Studio 2022 with the "Desktop development with C++" workload (v143 toolset). For `ARM64`
  also install "MSVC v143 - VS 2022 C++ ARM64 build tools".
- Windows 10 or 11 SDK, any recent version.

C++20 for C++ sources and C17 for C. No modules or header units; everything goes through the
precompiled header `stdafx.h`.

The CRT is linked statically (`/MT`, `/MTd`), so the exe runs from a game folder without a
Visual C++ redistributable.

## Layout

```
EndfieldLauncher.sln
Directory.Build.props             SolutionDir fallback for single-project builds
Directory.Build.targets           /SAFESEH only on Win32
Source\
    EndfieldLauncher.Common.props OutDir, IntDir, language standard, runtime library
    EndfieldLauncher\
        EntryPoint.cpp            wWinMain, calls Launcher::Run
        Launcher                  start-up sequence: options, game lookup, parent, wait
        Options                   command-line options and the --help text
        DecoyParent               the suspended Games.exe the game sees as its parent
        GameLocator               finds Endfield.exe outside the launcher's folder
        OfficialLauncher          Games.exe of an installed official launcher
        Shortcut                  desktop shortcut written by --install
        SignatureCheck            Authenticode and publisher check
        Core                      OS and file-system helpers
        DebugLog                  diagnostic log
        UserMessage               message boxes
        UniqueHandle.h            handle deleter
        stdafx.h / stdafx.cpp     precompiled header
        Resource.h / Resource.rc  icon and version resource
        res\                      icon and manifest
Build\                            binaries and PDBs (generated)
Intermediate\                     object files (generated)
```

One class per component: constants and helpers live in the header, the `.cpp` has implementations
only. PascalCase, tabs, CRLF. Every `.cpp` starts with `#include "stdafx.h"`.

`EndfieldLauncher.Common.props` is imported by every project; new projects under `Source\` should
import it the same way and not repeat its settings.

## Configurations

| Configuration | Use |
|---|---|
| `Release` | Shipping build: whole-program optimisation, COMDAT folding. |
| `Debug` | Development. |
| `AddressSanitizerDebug` | `Debug` with AddressSanitizer (`x64` and `Win32`; the ARM64 compiler has no AddressSanitizer, so there it is a plain Debug-style build). |

Platforms: `x64` (ship this, the game is x64), `ARM64` (native on Windows on ARM, starts the x64
game through emulation) and `Win32`.

Output: `Build\<Platform>\<Configuration>\EndfieldLauncher.exe` and its PDB.

## Command line

From a Developer Command Prompt:

```bat
msbuild EndfieldLauncher.sln /p:Configuration=Release /p:Platform=x64 /m
```

The project also builds alone; `Directory.Build.props` keeps `Build\` and `Intermediate\` in the
repository root:

```bat
msbuild Source\EndfieldLauncher\EndfieldLauncher.vcxproj /p:Configuration=Release /p:Platform=x64
```

## Where settings live

| Setting | Where |
|---|---|
| Administrator rights (UAC prompt) | `UACExecutionLevel` in the project |
| Per-monitor DPI awareness | `Manifest > EnableDpiAwareness` in the project |
| Supported OS (Windows 10 and 11) | `res\EndfieldLauncher.manifest` |
| Import libraries | `AdditionalDependencies` in the project |
| Desktop shortcut name and tooltip | `Shortcut::FileName`, `Shortcut::Description` |
| Remembered game path | `HKCU\Software\EndfieldLauncher`, value `GamePath` |

## Common errors

- `MSB8020: build tools for v143 cannot be found`: install the v143 toolset or retarget the project.
- `ARM64` missing or fails to link: install the ARM64 build tools component.
- `C1010: unexpected end of file while looking for precompiled header`: a source file doesn't
  include `stdafx.h` first.
