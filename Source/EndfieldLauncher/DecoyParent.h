#pragma once

#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "Options.h"
#include "UniqueHandle.h"

// The process the game sees as its parent. The game only checks its direct parent: it has to be
// named Games.exe and carry a valid Hypergryph signature. One is started suspended, so it never
// runs, and is passed as PROC_THREAD_ATTRIBUTE_PARENT_PROCESS when the game starts.
//
// It has to stay alive as long as the game does. Release (also run by the destructor) kills it and
// deletes any staged copy.
class DecoyParent {
public:
	// The name the game's parent check accepts, and the name of the staged copy.
	static constexpr std::wstring_view ParentExeName = L"Games.exe";

	// Signed files looked for in the game folder, in order.
	static constexpr std::array<std::wstring_view, 3> DecoyCandidates{
		L"PlatformProcess.exe", L"UnityCrashHandler64.exe", L"Endfield.exe" };

	// Delete retries and the pause between them: the file stays locked for a moment after the
	// process dies.
	static constexpr int RemoveAttempts = 3;
	static constexpr DWORD RemoveRetryMs = 150;

	// How long Release waits for the killed parent to exit before deleting its file.
	static constexpr DWORD TerminateWaitMs = 2000;

	DecoyParent() = default;
	DecoyParent(const DecoyParent&) = delete;
	DecoyParent& operator=(const DecoyParent&) = delete;
	~DecoyParent() { Release(); }

	// Picks and starts the parent, trying in this order:
	//   1. --parent <path>, in place
	//   2. the installed official Games.exe, in place (skipped with --no-official)
	//   3. a signed file from the game folder, copied to <DecoyDir>\Games.exe
	// On failure Failure holds the message box text; the details are in the log.
	bool Create(const fs::path& GameDir, const fs::path& DecoyDir, const Options& Opt, std::wstring& Failure);

	// Starts GamePath with the suspended parent as its parent. Returns null with Failure set on error.
	UniqueHandle StartGame(const fs::path& GamePath, const std::wstring& Args, std::wstring& Failure) const;

	// Pid of the parent, 0 if there is none.
	DWORD Pid() const { return Process ? GetProcessId(Process) : 0; }

	// Kills the parent and removes the staged file.
	void Release();

private:
	struct Candidate {
		fs::path Path;
		bool InPlace;       // run where it is; otherwise copied to DecoyPath first
		std::wstring Label; // for the log
	};

	static std::vector<Candidate> GatherCandidates(const fs::path& GameDir, const Options& Opt);

	// Copies Source if it isn't run in place, then starts it suspended. Keeps the handles on
	// success; on failure the copy is removed.
	bool StartCandidate(const Candidate& Source);

	void RemoveFile() const;

	HANDLE Process = nullptr;
	HANDLE Thread = nullptr;
	fs::path DecoyDirectory;
	fs::path DecoyPath;
};
