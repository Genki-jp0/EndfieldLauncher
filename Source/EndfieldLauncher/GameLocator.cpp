#include "stdafx.h"
#include "GameLocator.h"
#include "Core.h"
#include "DebugLog.h"
#include "Launcher.h"
#include "OfficialLauncher.h"

std::optional<fs::path> GameLocator::LoadSaved()
{
	wchar_t Buffer[MAX_PATH * 2]{};
	DWORD Size = sizeof(Buffer);
	if (RegGetValueW(HKEY_CURRENT_USER, RegistryKey, RegistryValue, RRF_RT_REG_SZ, nullptr, Buffer, &Size) != ERROR_SUCCESS)
		return std::nullopt;

	const fs::path Path = Buffer;
	std::error_code Error;
	if (!fs::is_regular_file(Path, Error))
		return std::nullopt;
	return Path;
}

void GameLocator::Save(const fs::path& Path)
{
	HKEY Key = nullptr;
	if (RegCreateKeyExW(HKEY_CURRENT_USER, RegistryKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &Key, nullptr) != ERROR_SUCCESS)
		return;
	const std::wstring Text = Path.wstring();
	RegSetValueExW(Key, RegistryValue, 0, REG_SZ, reinterpret_cast<const BYTE*>(Text.c_str()),
		static_cast<DWORD>((Text.size() + 1) * sizeof(wchar_t)));
	RegCloseKey(Key);
}

std::optional<fs::path> GameLocator::FromOfficialLauncher(const fs::path& GameDir)
{
	for (const fs::path& Exe : OfficialLauncher::FindExecutables(GameDir)) {
		const fs::path Folder = Exe.parent_path();
		// The launcher's own folder and its parent; going higher would scan far too much.
		for (const fs::path& Root : { Folder, Folder.parent_path() }) {
			if (Root.empty())
				continue;
			DebugLog::Write(L"looking for " + std::wstring(Launcher::GameExeName) + L" near " + Root.wstring());
			// Default install layout: <official root>\games\EndField Game\Endfield.exe.
			const fs::path Default = Root / L"games" / L"EndField Game" / Launcher::GameExeName;
			std::error_code DefaultError;
			if (fs::is_regular_file(Default, DefaultError))
				return Default;
			if (auto Found = Core::FindFile(Root, Launcher::GameExeName))
				return Found;
		}
	}
	return std::nullopt;
}

std::optional<fs::path> GameLocator::Ask()
{
	wchar_t Buffer[MAX_PATH * 2]{};
	OPENFILENAMEW Dialog{};
	Dialog.lStructSize = sizeof(Dialog);
	Dialog.lpstrFilter = L"Endfield.exe\0Endfield.exe\0";
	Dialog.lpstrFile = Buffer;
	Dialog.nMaxFile = static_cast<DWORD>(sizeof(Buffer) / sizeof(Buffer[0]));
	Dialog.lpstrTitle = L"Select Endfield.exe";
	Dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	if (!GetOpenFileNameW(&Dialog)) {
		DebugLog::Write(L"file dialog cancelled");
		return std::nullopt;
	}

	const fs::path Path = Buffer;
	if (!Core::IEquals(Path.filename().wstring(), Launcher::GameExeName)) {
		DebugLog::Write(L"not Endfield.exe: " + Path.wstring());
		return std::nullopt;
	}
	return Path;
}

std::optional<fs::path> GameLocator::Find(const fs::path& GameDir, bool ForcePick)
{
	if (!ForcePick) {
		if (auto Saved = LoadSaved()) {
			DebugLog::Write(L"using remembered game path " + Saved->wstring());
			return Saved;
		}
		if (auto Official = FromOfficialLauncher(GameDir)) {
			DebugLog::Write(L"found game near the official launcher: " + Official->wstring());
			Save(*Official);
			return Official;
		}
	}

	DebugLog::Write(L"asking for " + std::wstring(Launcher::GameExeName));
	auto Picked = Ask();
	if (Picked)
		Save(*Picked);
	return Picked;
}
