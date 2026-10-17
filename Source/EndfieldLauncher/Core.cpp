#include "stdafx.h"
#include "Core.h"
#include "UniqueHandle.h"

fs::path Core::SelfPath()
{
	std::wstring Buffer(MAX_PATH, L'\0');
	for (;;) {
		const DWORD Length = GetModuleFileNameW(nullptr, Buffer.data(), static_cast<DWORD>(Buffer.size()));
		if (Length == 0)
			return fs::path();
		if (Length < Buffer.size()) {
			Buffer.resize(Length);
			break;
		}
		Buffer.resize(Buffer.size() * 2);
	}
	return fs::path(Buffer);
}

fs::path Core::SelfDirectory()
{
	const fs::path Self = SelfPath();
	return Self.empty() ? fs::current_path() : Self.parent_path();
}

fs::path Core::DefaultDecoyDirectory()
{
	std::wstring Buffer(MAX_PATH + 1, L'\0');
	const DWORD Length = GetTempPathW(static_cast<DWORD>(Buffer.size()), Buffer.data());
	Buffer.resize(Length);
	return fs::path(Buffer) / L"EndfieldLauncher" / L"ParentBridge";
}

std::wstring Core::LastErrorText(DWORD Error)
{
	wchar_t* Buffer = nullptr;
	const DWORD Length = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
		Error, 0, reinterpret_cast<LPWSTR>(&Buffer), 0, nullptr);
	std::wstring Text = Length ? std::wstring(Buffer, Length) : L"Unknown error";
	if (Buffer)
		LocalFree(Buffer);
	while (!Text.empty() && (Text.back() == L'\r' || Text.back() == L'\n' || Text.back() == L' '))
		Text.pop_back();
	return Text + L" (" + std::to_wstring(Error) + L")";
}

bool Core::IEquals(std::wstring_view Left, std::wstring_view Right)
{
	return CompareStringOrdinal(Left.data(), static_cast<int>(Left.size()),
		Right.data(), static_cast<int>(Right.size()), TRUE) == CSTR_EQUAL;
}

bool Core::IContains(std::wstring_view Haystack, std::wstring_view Needle)
{
	if (Needle.empty() || Haystack.size() < Needle.size())
		return false;
	for (size_t Start = 0; Start + Needle.size() <= Haystack.size(); ++Start) {
		if (IEquals(Haystack.substr(Start, Needle.size()), Needle))
			return true;
	}
	return false;
}

bool Core::IsReparsePoint(const fs::path& Path)
{
	const DWORD Attributes = GetFileAttributesW(Path.c_str());
	return Attributes == INVALID_FILE_ATTRIBUTES || (Attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

std::optional<fs::path> Core::FindFile(const fs::path& Root, std::wstring_view FileName)
{
	std::error_code Error;
	const fs::path Direct = Root / FileName;
	if (fs::is_regular_file(Direct, Error))
		return Direct;

	std::vector<fs::path> Current{ Root };
	for (int Depth = 0; Depth < SearchDepth && !Current.empty(); ++Depth) {
		std::vector<fs::path> Next;
		for (const fs::path& Directory : Current) {
			fs::directory_iterator Iterator(Directory, fs::directory_options::skip_permission_denied, Error);
			if (Error) {
				Error.clear();
				continue;
			}
			for (const fs::directory_iterator End; Iterator != End; Iterator.increment(Error)) {
				if (Error) {
					Error.clear();
					break;
				}
				std::error_code EntryError;
				if (!Iterator->is_directory(EntryError) || EntryError)
					continue;
				const fs::path Candidate = Iterator->path() / FileName;
				if (fs::is_regular_file(Candidate, EntryError))
					return Candidate;
				if (!IsReparsePoint(Iterator->path()))
					Next.push_back(Iterator->path());
			}
		}
		Current = std::move(Next);
	}
	return std::nullopt;
}

DWORD Core::ParentPidOf(DWORD Pid)
{
	const UniqueHandle Snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
	if (Snapshot.get() == INVALID_HANDLE_VALUE)
		return 0;
	PROCESSENTRY32W Entry{};
	Entry.dwSize = sizeof(Entry);
	for (BOOL Found = Process32FirstW(Snapshot.get(), &Entry); Found; Found = Process32NextW(Snapshot.get(), &Entry)) {
		if (Entry.th32ProcessID == Pid)
			return Entry.th32ParentProcessID;
	}
	return 0;
}
