#include "stdafx.h"
#include "DebugLog.h"

DebugLog DebugLogObjects;

void DebugLog::Start(const fs::path& Path)
{
	std::error_code Error;
	fs::create_directories(Path.parent_path(), Error);
	const HANDLE File = CreateFileW(Path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, nullptr); // truncate previous log
	if (File != INVALID_HANDLE_VALUE)
		CloseHandle(File);
	DebugLogObjects.LogPath = Path;
}

void DebugLog::Write(const std::wstring& Message)
{
	if (DebugLogObjects.LogPath.empty())
		return;

	SYSTEMTIME Time;
	GetLocalTime(&Time);
	wchar_t Stamp[40];
	wsprintfW(Stamp, L"%02u:%02u:%02u.%03u ", Time.wHour, Time.wMinute, Time.wSecond, Time.wMilliseconds);
	const std::wstring Line = std::wstring(Stamp) + Message + L"\r\n";
	const int Size = WideCharToMultiByte(CP_UTF8, 0, Line.data(), static_cast<int>(Line.size()), nullptr, 0, nullptr, nullptr);
	if (Size <= 0)
		return;
	std::string Utf8(static_cast<size_t>(Size), '\0');
	WideCharToMultiByte(CP_UTF8, 0, Line.data(), static_cast<int>(Line.size()), Utf8.data(), Size, nullptr, nullptr);
	const HANDLE File = CreateFileW(DebugLogObjects.LogPath.c_str(), FILE_APPEND_DATA,
		FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (File == INVALID_HANDLE_VALUE)
		return;
	DWORD Written = 0;
	WriteFile(File, Utf8.data(), static_cast<DWORD>(Utf8.size()), &Written, nullptr);
	CloseHandle(File);
}
