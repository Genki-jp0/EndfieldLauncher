#pragma once

#include <optional>
#include <string>

// Finds Endfield.exe when it isn't next to the launcher, e.g. the game is on another drive or the
// launcher lives somewhere else.
class GameLocator {
public:
	// Where the chosen path is remembered (HKCU).
	static constexpr const wchar_t* RegistryKey = L"Software\\EndfieldLauncher";
	static constexpr const wchar_t* RegistryValue = L"GamePath";

	// Tries, in order: the remembered path, the neighbourhood of the installed official launcher,
	// a file dialog. A path found by the last two is remembered. ForcePick goes straight to the
	// dialog. Returns nothing if the user cancels.
	static std::optional<fs::path> Find(const fs::path& GameDir, bool ForcePick);

private:
	// Remembered path, if the file still exists.
	static std::optional<fs::path> LoadSaved();
	static void Save(const fs::path& Path);

	// Looks for Endfield.exe near each installed official launcher. The install layout isn't
	// guaranteed, so this is best effort.
	static std::optional<fs::path> FromOfficialLauncher(const fs::path& GameDir);

	// "Open" dialog; only a file named Endfield.exe is accepted.
	static std::optional<fs::path> Ask();
};
