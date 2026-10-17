#pragma once

#include <array>
#include <string>
#include <string_view>
#include <vector>

// Finds the Games.exe of an installed official launcher (Hypergryph or Gryphline). It already has
// the right name and signature, so it can be used in place and nothing needs copying.
class OfficialLauncher {
public:
	// Parent folders of the game folder searched for Games.exe.
	static constexpr int AncestorLevels = 6;

	// The closest ancestors also get a two-level scan downwards, because the launcher usually
	// sits in a sibling of the games folder.
	static constexpr int SiblingScanLevels = 2;

	// URL protocol handlers the official launchers register under Software\Classes.
	static constexpr std::array<std::wstring_view, 2> ProtocolPrefixes{ L"hypergryphlauncher", L"gryphlinelauncher" };

	// Games.exe files from the ancestors of GameDir and the registered protocol handlers. Existing
	// files only, no duplicates.
	static std::vector<fs::path> FindExecutables(const fs::path& GameDir);

private:
	// REG_SZ or REG_EXPAND_SZ value without terminators; empty if missing or another type.
	static std::wstring ReadRegString(HKEY Key, const wchar_t* ValueName);

	// Exe path from a shell command such as: "C:\X\Games.exe" "%1"
	static std::wstring ExeFromCommand(const std::wstring& Command);
};
