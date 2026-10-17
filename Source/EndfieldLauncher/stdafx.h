// Precompiled header. Every source file includes it first.

#pragma once

#include "targetver.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Import libraries for these (wintrust, crypt32, shell32, user32, advapi32, comdlg32, ole32, uuid)
// are in the vcxproj under AdditionalDependencies.
#include <windows.h>
#include <wincrypt.h>
#include <commdlg.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <softpub.h>
#include <tlhelp32.h>
#include <wintrust.h>

#include <array>
#include <cwctype>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;
