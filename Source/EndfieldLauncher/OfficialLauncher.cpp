#include "stdafx.h"
#include "OfficialLauncher.h"
#include "Core.h"
#include "DecoyParent.h"

std::wstring OfficialLauncher::ReadRegString(HKEY Key, const wchar_t* ValueName)
{
	DWORD Type = 0, Size = 0;
	if (RegQueryValueExW(Key, ValueName, nullptr, &Type, nullptr, &Size) != ERROR_SUCCESS ||
		(Type != REG_SZ && Type != REG_EXPAND_SZ) || Size < sizeof(wchar_t))
		return {};
	std::wstring Value(Size / sizeof(wchar_t), L'\0');
	if (RegQueryValueExW(Key, ValueName, nullptr, &Type, reinterpret_cast<LPBYTE>(Value.data()), &Size) != ERROR_SUCCESS)
		return {};
	while (!Value.empty() && Value.back() == L'\0')
		Value.pop_back();
	return Value;
}

std::wstring OfficialLauncher::ExeFromCommand(const std::wstring& Command)
{
	if (Command.empty())
		return {};
	if (Command[0] == L'"') {
		const size_t End = Command.find(L'"', 1);
		return End == std::wstring::npos ? std::wstring{} : Command.substr(1, End - 1);
	}
	const size_t Exe = Command.find(L".exe");
	return Exe == std::wstring::npos ? std::wstring{} : Command.substr(0, Exe + 4);
}

std::vector<fs::path> OfficialLauncher::FindExecutables(const fs::path& GameDir)
{
	std::vector<fs::path> Found;
	auto Add = [&](const fs::path& Path) {
		std::error_code Error;
		if (!fs::is_regular_file(Path, Error))
			return;
		for (const fs::path& Existing : Found) {
			if (Core::IEquals(Existing.wstring(), Path.wstring()))
				return;
		}
		Found.push_back(Path);
	};

	fs::path Directory = GameDir;
	for (int Level = 0; Level < AncestorLevels && Directory.has_parent_path() && Directory != Directory.parent_path(); ++Level) {
		Directory = Directory.parent_path();
		Add(Directory / DecoyParent::ParentExeName);

		// Closest ancestors are also scanned two levels down; the launcher usually sits in a sibling folder.
		if (Level < SiblingScanLevels) {
			std::error_code Error;
			for (const auto& Child : fs::directory_iterator(Directory, fs::directory_options::skip_permission_denied, Error)) {
				std::error_code ChildError;
				if (!Child.is_directory(ChildError) || Core::IsReparsePoint(Child.path()))
					continue;
				Add(Child.path() / DecoyParent::ParentExeName);
				for (const auto& Grandchild : fs::directory_iterator(Child.path(), fs::directory_options::skip_permission_denied, ChildError)) {
					std::error_code GrandchildError;
					if (Grandchild.is_directory(GrandchildError) && !Core::IsReparsePoint(Grandchild.path()))
						Add(Grandchild.path() / DecoyParent::ParentExeName);
				}
			}
		}
	}

	for (const HKEY Root : { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE }) {
		HKEY Classes = nullptr;
		if (RegOpenKeyExW(Root, L"Software\\Classes", 0, KEY_READ, &Classes) != ERROR_SUCCESS)
			continue;
		for (DWORD Index = 0;; ++Index) {
			wchar_t Name[256];
			DWORD Length = 256;
			if (RegEnumKeyExW(Classes, Index, Name, &Length, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
				break;
			const std::wstring_view KeyName(Name, Length);
			bool Match = false;
			for (const std::wstring_view Prefix : ProtocolPrefixes) {
				if (KeyName.size() >= Prefix.size() && Core::IEquals(KeyName.substr(0, Prefix.size()), Prefix))
					Match = true;
			}
			if (!Match)
				continue;

			HKEY CommandKey = nullptr;
			const std::wstring SubKey = std::wstring(KeyName) + L"\\shell\\open\\command";
			if (RegOpenKeyExW(Classes, SubKey.c_str(), 0, KEY_READ, &CommandKey) != ERROR_SUCCESS)
				continue;
			const fs::path Exe = ExeFromCommand(ReadRegString(CommandKey, nullptr));
			RegCloseKey(CommandKey);
			if (Exe.empty())
				continue;
			Add(Exe);
			Add(Exe.parent_path() / DecoyParent::ParentExeName);
		}
		RegCloseKey(Classes);
	}
	return Found;
}
