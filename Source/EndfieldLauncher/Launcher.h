#pragma once

#include <string>
#include <string_view>

#include "DecoyParent.h"
#include "Options.h"

// EndfieldLauncher: tiny launcher for Arknights: Endfield.
//
// The game checks that its direct parent is called Games.exe and is signed by Hypergryph. Signed
// files already ship in the game folder, so:
//   1. one is copied to <decoy-dir>\Games.exe,
//   2. the copy is started suspended (it never runs),
//   3. Endfield.exe is started with that process as PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
//   4. when the game exits, the copy is killed and deleted.
// An installed official Games.exe is preferred and run in place; see DecoyParent::Create.
//
// The manifest asks for administrator rights, so a double click raises UAC.
//
// Run blocks until the game exits. Cleaning up earlier would remove the parent the game checks.
class Launcher {
public:
	static constexpr std::wstring_view GameExeName = L"Endfield.exe";

	// With --debug, a game that exits sooner than this is reported; it usually means the parent
	// check failed.
	static constexpr ULONGLONG EarlyExitMs = 15000;

	static constexpr int ExitOk = 0;
	static constexpr int ExitGameNotFound = 2;
	static constexpr int ExitNoParent = 3;
	static constexpr int ExitStartFailed = 4;
	static constexpr int ExitShortcutFailed = 5;

	// Parses the command line, starts the game under the signed parent and waits for it to exit.
	// Returns one of the Exit codes.
	static int Run();

private:
	// Writes the desktop shortcut over any existing one (--game from Opt is kept in it). Shows a
	// message box and returns false on failure.
	static bool CreateShortcut(const Options& Opt);

	// --install: creates the shortcut, tells the user and counts the question as answered for this
	// exe path. Returns ExitOk or ExitShortcutFailed.
	static int Install(const Options& Opt);

	// Asks Yes/No whether to create the desktop shortcut, once per exe path: the path is remembered
	// in HKCU whatever the answer, so the same location is never asked twice, while a copy in
	// another folder is asked again. If the desktop already has a shortcut to this exe, nothing is
	// asked; if it has one that points elsewhere, the question is whether to overwrite it.
	static void OfferShortcut(const Options& Opt);

	// Starts the log if --debug or --log asked for one, filling in Opt.LogPath.
	static void StartLogging(Options& Opt);

	// Opt.Game, Endfield.exe under Opt.GameDir, or whatever GameLocator finds (the game may be on
	// another drive). Shows a message box and returns false if there's none.
	static bool ResolveGamePath(const Options& Opt, fs::path& GamePath);

	// Where to look for signed files: the game's folder, or GameDir when the game is inside it.
	static fs::path ResolveSearchRoot(const fs::path& GamePath, const fs::path& GameDir);

	// Logs the parent the game actually got, waits for it to exit, and with --debug reports an early exit.
	static void WaitForGame(HANDLE Game, const DecoyParent& Decoy, const Options& Opt);
};
