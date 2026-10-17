#pragma once

#include <string>

// Message boxes, the only UI. There's no window, so failures before the game starts can only be
// shown this way.
class UserMessage {
public:
	static constexpr const wchar_t* Title = L"Endfield Launcher";

	static void ShowError(const std::wstring& Text);
	static void ShowInfo(const wchar_t* Text);

	// Yes/No question; true for Yes. Closing the box counts as No.
	static bool AskYesNo(const wchar_t* Text);
};
