#pragma once

#include <string>

// Desktop shortcut to the launcher: created by --install and by the first-run question.
class Shortcut {
public:
	// File name on the desktop.
	static constexpr const wchar_t* FileName = L"Endfield Launcher.lnk";

	// Description shown in the shortcut's tooltip.
	static constexpr const wchar_t* Description = L"Start Arknights: Endfield";

	// REG_SZ under HKCU\GameLocator::RegistryKey: the full path of the exe the shortcut question
	// was last shown for. The same exe path is never asked about twice, whatever the answer was; a
	// copy in another folder gets its own question.
	static constexpr const wchar_t* PromptedValue = L"ShortcutPromptedFor";

	// What is on the desktop under FileName, compared with a given exe.
	enum class State {
		Missing,   // no such shortcut (or the desktop can't be found)
		Matches,   // it exists and its target is that exe
		Different, // it exists but points elsewhere, or can't be read
	};

	// True if the question was already shown for this exe path (or --install made the shortcut for
	// it). Same file, so case, 8.3 names and links don't matter.
	static bool WasPromptedFor(const fs::path& Exe);

	// Records that the question was shown for Exe. Best effort: if the registry can't be written
	// the question comes back next run.
	static void MarkPrompted(const fs::path& Exe);

	// Looks at <desktop>\Endfield Launcher.lnk and compares its target with Target (same file, so
	// case, 8.3 names and links don't matter). Arguments and icon are not compared.
	static State Inspect(const fs::path& Target);

	// Writes <desktop>\Endfield Launcher.lnk pointing at Target, replacing any shortcut that is
	// already there (read-only or not). The icon is Target's own. Arguments go on the shortcut's
	// command line as is. The desktop is the current user's (a redirected one, e.g. OneDrive, is
	// followed). On failure returns false and fills Failure with text for a message box.
	static bool CreateOnDesktop(const fs::path& Target, const std::wstring& Arguments, std::wstring& Failure);
};
