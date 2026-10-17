#include "stdafx.h"
#include "Launcher.h"
#include "Core.h"
#include "DebugLog.h"
#include "GameLocator.h"
#include "Shortcut.h"
#include "UserMessage.h"

bool Launcher::CreateShortcut(const Options& Opt)
{
	std::wstring Arguments;
	if (!Opt.Game.empty()) {
		std::error_code Error;
		const fs::path Game = fs::absolute(Opt.Game, Error);
		Arguments = L"--game \"" + (Error ? Opt.Game : Game).wstring() + L"\"";
	}

	std::wstring Failure;
	if (!Shortcut::CreateOnDesktop(Core::SelfPath(), Arguments, Failure)) {
		DebugLog::Write(L"shortcut failed: " + Failure);
		UserMessage::ShowError(Failure);
		return false;
	}
	DebugLog::Write(L"desktop shortcut written");
	return true;
}

int Launcher::Install(const Options& Opt)
{
	if (!CreateShortcut(Opt))
		return ExitShortcutFailed;
	Shortcut::MarkPrompted(Core::SelfPath());
	UserMessage::ShowInfo(L"Desktop shortcut created.");
	return ExitOk;
}

void Launcher::OfferShortcut(const Options& Opt)
{
	const fs::path Self = Core::SelfPath();
	if (Shortcut::WasPromptedFor(Self))
		return;

	const Shortcut::State State = Shortcut::Inspect(Self);
	if (State == Shortcut::State::Matches) {
		// Already set up for this exe: nothing to ask, and nothing was shown, so nothing is recorded.
		DebugLog::Write(L"desktop shortcut already points here");
		return;
	}

	const bool Replace = State == Shortcut::State::Different;
	const bool Yes = UserMessage::AskYesNo(Replace
		? L"A desktop shortcut named \"Endfield Launcher\" already exists but points somewhere else.\n\n"
		  L"Overwrite it with a shortcut to this launcher?\n\nThis is only asked once for this location."
		: L"Create a desktop shortcut for Endfield Launcher?\n\nThis is only asked once for this location.");
	Shortcut::MarkPrompted(Self);
	DebugLog::Write(std::wstring(Replace ? L"shortcut overwrite offer: " : L"shortcut offer: ") + (Yes ? L"yes" : L"no"));
	if (Yes)
		CreateShortcut(Opt);
}

void Launcher::StartLogging(Options& Opt)
{
	if (!Opt.Debug && Opt.LogPath.empty())
		return;

	if (Opt.LogPath.empty())
		Opt.LogPath = Core::DefaultDecoyDirectory().parent_path() / L"launcher.log";
	DebugLog::Start(Opt.LogPath);
	DebugLog::Write(L"EndfieldLauncher started. gameDir=" + Opt.GameDir.wstring() + L" decoyDir=" + Opt.DecoyDir.wstring());
}

bool Launcher::ResolveGamePath(const Options& Opt, fs::path& GamePath)
{
	GamePath = Opt.Game;
	if (GamePath.empty()) {
		std::optional<fs::path> Found;
		if (!Opt.Pick)
			Found = Core::FindFile(Opt.GameDir, GameExeName);
		if (!Found)
			Found = GameLocator::Find(Opt.GameDir, Opt.Pick);
		if (!Found) {
			UserMessage::ShowError(L"Endfield.exe not found.\n\nPut the launcher in the game folder, "
				L"pick the file when asked, or use --game <path>.");
			return false;
		}
		GamePath = *Found;
	}

	std::error_code Error;
	if (!fs::is_regular_file(GamePath, Error)) {
		UserMessage::ShowError(L"Game not found:\n" + GamePath.wstring());
		return false;
	}
	GamePath = fs::absolute(GamePath, Error);
	return true;
}

fs::path Launcher::ResolveSearchRoot(const fs::path& GamePath, const fs::path& GameDir)
{
	fs::path SearchRoot = GamePath.parent_path();
	std::error_code Error;
	const fs::path Relative = fs::relative(GamePath, GameDir, Error);
	if (!Error && !Relative.empty() && *Relative.begin() != L"..")
		SearchRoot = GameDir;
	return SearchRoot;
}

void Launcher::WaitForGame(HANDLE Game, const DecoyParent& Decoy, const Options& Opt)
{
	const DWORD GamePid = GetProcessId(Game);
	const DWORD ParentPid = Core::ParentPidOf(GamePid);
	DebugLog::Write(L"game started, pid " + std::to_wstring(GamePid) + L", parent pid " + std::to_wstring(ParentPid) +
		L" (decoy pid " + std::to_wstring(Decoy.Pid()) + L", " +
		(ParentPid == Decoy.Pid() ? L"parent spoof OK" : L"PARENT MISMATCH") + L")");

	const ULONGLONG StartTick = GetTickCount64();
	WaitForSingleObject(Game, INFINITE);
	DWORD ExitCode = 0;
	GetExitCodeProcess(Game, &ExitCode);
	const ULONGLONG RunMs = GetTickCount64() - StartTick;
	DebugLog::Write(L"game exited with code " + std::to_wstring(ExitCode) + L" after " + std::to_wstring(RunMs) + L" ms");
	if (Opt.Debug && RunMs < EarlyExitMs)
		UserMessage::ShowError(L"Game exited after " + std::to_wstring(RunMs) + L" ms (code " + std::to_wstring(ExitCode) +
			L").\n\nLog:\n" + Opt.LogPath.wstring());
}

int Launcher::Run()
{
	Options Opt = Options::Parse();
	if (Opt.Help) {
		UserMessage::ShowInfo(Options::Usage);
		return ExitOk;
	}
	if (Opt.Install)
		return Install(Opt);

	if (Opt.GameDir.empty())
		Opt.GameDir = Core::SelfDirectory();
	if (Opt.DecoyDir.empty())
		Opt.DecoyDir = Core::DefaultDecoyDirectory();
	StartLogging(Opt);

	fs::path GamePath;
	if (!ResolveGamePath(Opt, GamePath))
		return ExitGameNotFound;

	// Only once the game was found: the launcher is in a working place by now.
	OfferShortcut(Opt);

	const fs::path SearchRoot = ResolveSearchRoot(GamePath, Opt.GameDir);

	DecoyParent Decoy;
	std::wstring Failure;
	DebugLog::Write(L"game=" + GamePath.wstring() + L" searchRoot=" + SearchRoot.wstring());
	if (!Decoy.Create(SearchRoot, Opt.DecoyDir, Opt, Failure)) {
		UserMessage::ShowError(Failure);
		return ExitNoParent;
	}

	UniqueHandle Game = Decoy.StartGame(GamePath, Opt.GameArgs, Failure);
	if (!Game) {
		DebugLog::Write(L"StartGame failed: " + Failure);
		UserMessage::ShowError(Failure);
		return ExitStartFailed;
	}

	// The parent has to outlive the game.
	WaitForGame(Game.get(), Decoy, Opt);
	Decoy.Release();
	return ExitOk;
}
