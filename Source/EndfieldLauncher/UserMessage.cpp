#include "stdafx.h"
#include "UserMessage.h"

void UserMessage::ShowError(const std::wstring& Text)
{
	MessageBoxW(nullptr, Text.c_str(), Title, MB_OK | MB_ICONERROR);
}

void UserMessage::ShowInfo(const wchar_t* Text)
{
	MessageBoxW(nullptr, Text, Title, MB_OK | MB_ICONINFORMATION);
}

bool UserMessage::AskYesNo(const wchar_t* Text)
{
	return MessageBoxW(nullptr, Text, Title, MB_YESNO | MB_DEFBUTTON1 | MB_ICONQUESTION) == IDYES;
}
