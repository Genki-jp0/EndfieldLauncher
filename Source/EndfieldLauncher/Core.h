#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

// OS and file-system helpers. None of them log or show UI; the caller decides what a failure means.
class Core {
public:
	// Directory levels FindFile descends below its root.
	static constexpr int SearchDepth = 3;

	// Full path of this exe; empty if the module path can't be read.
	static fs::path SelfPath();

	// Folder of this exe; the current directory if the module path can't be read.
	static fs::path SelfDirectory();

	// %TEMP%\EndfieldLauncher\ParentBridge
	static fs::path DefaultDecoyDirectory();

	// FormatMessage text without the trailing newline, plus the number in parentheses.
	static std::wstring LastErrorText(DWORD Error);

	// Ordinal and case-insensitive, so the result doesn't depend on the user's locale.
	static bool IEquals(std::wstring_view Left, std::wstring_view Right);

	// Case-insensitive substring test. An empty Needle never matches.
	static bool IContains(std::wstring_view Haystack, std::wstring_view Needle);

	// True for junctions and symlinks, and when the attributes can't be read. Directory walks skip
	// these so a link can't lead out of the game folder or loop.
	static bool IsReparsePoint(const fs::path& Path);

	// Breadth-first search for FileName under Root, SearchDepth levels at most. Skips reparse points.
	static std::optional<fs::path> FindFile(const fs::path& Root, std::wstring_view FileName);

	// Parent pid of Pid from a toolhelp snapshot, 0 if Pid isn't found.
	static DWORD ParentPidOf(DWORD Pid);
};
