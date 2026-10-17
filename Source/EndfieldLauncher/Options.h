#pragma once

#include <string>

// Command-line options. Every path has a default so a double click works; Launcher::Run fills them
// in after Parse. Unknown arguments are ignored.
class Options {
public:
	// --game <path to Endfield.exe>. Default: search next to this exe.
	fs::path Game;

	// --game-dir <folder>. Default: the folder of this exe.
	fs::path GameDir;

	// --decoy-dir <folder>. Default: %TEMP%\EndfieldLauncher\ParentBridge.
	fs::path DecoyDir;

	// --args "<text>", or everything after "--". Passed to the game as is.
	std::wstring GameArgs;

	// --log <file>. With --debug and no --log: %TEMP%\EndfieldLauncher\launcher.log.
	fs::path LogPath;

	// --parent <path>. Signed exe to run in place as the parent, e.g. the real Games.exe.
	fs::path ParentExe;

	// --debug. Write the log and show the "game exited early" box.
	bool Debug = false;

	// --pick. Choose Endfield.exe in a file dialog and remember it.
	bool Pick = false;

	// --no-official. Don't look for the installed official launcher.
	bool NoOfficial = false;

	// --decoy-source <name|path>. Use this signed file as the parent.
	std::wstring DecoySource;

	// --install. Create (or replace) the desktop shortcut to this exe and exit without starting
	// the game. --game, if given, is baked into the shortcut.
	bool Install = false;

	// --help, -h or /?
	bool Help = false;

	// Reads the process command line. A value option that is last on the line is ignored.
	static Options Parse();

	// Text of the --help box.
	static constexpr const wchar_t* Usage =
		L"Endfield Launcher\n\n"
		L"Double click to start Endfield.exe from this folder (expect a UAC prompt).\n\n"
		L"Options:\n"
		L"  --game <path>        Full path to Endfield.exe\n"
		L"  --game-dir <dir>     Folder to search for Endfield.exe (default: this exe's folder)\n"
		L"  --decoy-dir <dir>    Where the temporary Games.exe goes\n"
		L"                       (default: %TEMP%\\EndfieldLauncher\\ParentBridge)\n"
		L"  --log <file>         Write a log here (--debug implies it; default: %TEMP%\\EndfieldLauncher\\launcher.log)\n"
		L"  --parent <path>      Run this signed exe in place as the parent (e.g. the real Games.exe)\n"
		L"  --pick               Choose Endfield.exe in a dialog and remember it\n"
		L"  --debug              Write a log and warn if the game exits within 15 s\n"
		L"  --no-official        Skip the installed official Games.exe\n"
		L"  --decoy-source <x>   Use one signed file (name or full path) as the parent\n"
		L"  --install           Create the desktop shortcut (replaces an existing one) and exit\n"
		L"  --args \"<text>\"      Arguments for the game (or put them after \"--\")\n"
		L"  --help               Show this text";
};
