#pragma once

#include <string>

// Per-run diagnostic log: UTF-8, CRLF, millisecond timestamp on each line. Does nothing until
// Start is called (--debug or --log), so call sites don't need a guard.
class DebugLog {
public:
	// Empty while logging is off.
	fs::path LogPath;

	// Creates the folder, truncates any old log and starts logging to Path.
	static void Start(const fs::path& Path);

	// Appends a line. If the file can't be opened the line is dropped; a log line isn't worth
	// failing the launch.
	static void Write(const std::wstring& Message);
};

extern DebugLog DebugLogObjects;
