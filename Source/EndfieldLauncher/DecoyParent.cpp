#include "stdafx.h"
#include "DecoyParent.h"
#include "Core.h"
#include "DebugLog.h"
#include "OfficialLauncher.h"
#include "SignatureCheck.h"

std::vector<DecoyParent::Candidate> DecoyParent::GatherCandidates(const fs::path& GameDir, const Options& Opt)
{
	std::vector<Candidate> Candidates;

	if (!Opt.ParentExe.empty())
		Candidates.push_back({ Opt.ParentExe, true, L"--parent" });
	if (Opt.ParentExe.empty() && Opt.DecoySource.empty() && !Opt.NoOfficial) {
		auto Official = OfficialLauncher::FindExecutables(GameDir);
		if (Official.empty())
			DebugLog::Write(L"no official launcher Games.exe found near " + GameDir.wstring() + L" or in the registry");
		for (auto& Path : Official)
			Candidates.push_back({ std::move(Path), true, L"official launcher" });
	}

	if (Opt.ParentExe.empty()) {
		std::vector<std::wstring> Names;
		if (!Opt.DecoySource.empty())
			Names.push_back(Opt.DecoySource);
		else {
			for (const std::wstring_view Name : DecoyCandidates)
				Names.emplace_back(Name);
		}

		for (const std::wstring& Name : Names) {
			std::error_code ExistsError;
			std::optional<fs::path> Source;
			if (fs::path(Name).has_parent_path() && fs::is_regular_file(Name, ExistsError))
				Source = fs::path(Name);
			else
				Source = Core::FindFile(GameDir, fs::path(Name).filename().wstring());
			if (!Source) {
				DebugLog::Write(L"candidate " + Name + L": not found under " + GameDir.wstring());
				continue;
			}
			Candidates.push_back({ *Source, false, L"decoy copy of " + Name });
		}
	}

	return Candidates;
}

bool DecoyParent::StartCandidate(const Candidate& Source)
{
	fs::path Exe = Source.Path;
	if (!Source.InPlace) {
		std::error_code Error;
		fs::create_directories(DecoyDirectory, Error);
		fs::copy_file(Source.Path, DecoyPath, fs::copy_options::overwrite_existing, Error);
		if (Error) {
			DebugLog::Write(L"  cannot stage copy: " + std::wstring(Error.message().begin(), Error.message().end()));
			return false;
		}
		SetFileAttributesW(DecoyPath.c_str(), FILE_ATTRIBUTE_NORMAL); // read-only flag would block cleanup
		Exe = DecoyPath;
	}

	STARTUPINFOW StartupInfo{};
	StartupInfo.cb = sizeof(StartupInfo);
	PROCESS_INFORMATION ProcessInfo{};
	const std::wstring WorkingDirectory = Exe.parent_path().wstring();
	if (!CreateProcessW(Exe.c_str(), nullptr, nullptr, nullptr, FALSE, CREATE_SUSPENDED | CREATE_NO_WINDOW,
		nullptr, WorkingDirectory.c_str(), &StartupInfo, &ProcessInfo)) {
		DebugLog::Write(L"  cannot start suspended: " + Core::LastErrorText(GetLastError()));
		if (!Source.InPlace)
			RemoveFile();
		return false;
	}
	DebugLog::Write(L"  parent " + Exe.wstring() + L" running suspended, pid " + std::to_wstring(ProcessInfo.dwProcessId));
	Process = ProcessInfo.hProcess;
	Thread = ProcessInfo.hThread;
	return true;
}

bool DecoyParent::Create(const fs::path& GameDir, const fs::path& DecoyDir, const Options& Opt, std::wstring& Failure)
{
	DecoyDirectory = DecoyDir;
	DecoyPath = DecoyDir / ParentExeName;
	RemoveFile();

	for (const Candidate& Source : GatherCandidates(GameDir, Opt)) {
		DebugLog::Write(L"candidate [" + Source.Label + L"]: " + Source.Path.wstring());
		if (!SignatureCheck::IsTrustedFile(Source.Path))
			continue;
		if (StartCandidate(Source))
			return true;
	}

	Failure = L"No usable signed parent found (game folder: " + GameDir.wstring() +
		L").\nRun with --debug for details.";
	return false;
}

UniqueHandle DecoyParent::StartGame(const fs::path& GamePath, const std::wstring& Args, std::wstring& Failure) const
{
	SIZE_T AttributeSize = 0;
	InitializeProcThreadAttributeList(nullptr, 1, 0, &AttributeSize);
	std::vector<BYTE> AttributeBuffer(AttributeSize);
	auto* AttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(AttributeBuffer.data());
	if (AttributeList == nullptr || !InitializeProcThreadAttributeList(AttributeList, 1, 0, &AttributeSize)) {
		Failure = L"InitializeProcThreadAttributeList failed: " + Core::LastErrorText(GetLastError());
		return nullptr;
	}

	HANDLE Parent = Process;
	UniqueHandle Result;
	if (!UpdateProcThreadAttribute(AttributeList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &Parent, sizeof(Parent),
		nullptr, nullptr)) {
		Failure = L"UpdateProcThreadAttribute failed: " + Core::LastErrorText(GetLastError());
	}
	else {
		STARTUPINFOEXW StartupInfo{};
		StartupInfo.StartupInfo.cb = sizeof(StartupInfo);
		StartupInfo.lpAttributeList = AttributeList;

		std::wstring CommandLine = L"\"" + GamePath.wstring() + L"\"";
		if (!Args.empty())
			CommandLine += L" " + Args;

		const std::wstring WorkingDirectory = GamePath.parent_path().wstring();
		PROCESS_INFORMATION ProcessInfo{};
		if (!CreateProcessW(GamePath.c_str(), CommandLine.data(), nullptr, nullptr, FALSE,
			EXTENDED_STARTUPINFO_PRESENT, nullptr, WorkingDirectory.c_str(), &StartupInfo.StartupInfo, &ProcessInfo)) {
			Failure = L"CreateProcess failed: " + Core::LastErrorText(GetLastError());
		}
		else {
			CloseHandle(ProcessInfo.hThread);
			Result.reset(ProcessInfo.hProcess);
		}
	}
	DeleteProcThreadAttributeList(AttributeList);
	return Result;
}

void DecoyParent::Release()
{
	if (Process) {
		TerminateProcess(Process, 0);
		WaitForSingleObject(Process, TerminateWaitMs); // let the image mapping go before deleting the file
		CloseHandle(Process);
		Process = nullptr;
	}
	if (Thread) {
		CloseHandle(Thread);
		Thread = nullptr;
	}
	RemoveFile();
}

void DecoyParent::RemoveFile() const
{
	if (DecoyPath.empty())
		return;
	for (int Attempt = 0; Attempt < RemoveAttempts; ++Attempt) {
		std::error_code Error;
		if (!fs::exists(DecoyPath, Error))
			return;
		if (fs::remove(DecoyPath, Error))
			return;
		Sleep(RemoveRetryMs);
	}
}
