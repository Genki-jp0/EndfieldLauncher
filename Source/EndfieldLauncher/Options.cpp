#include "stdafx.h"
#include "Options.h"

Options Options::Parse()
{
	Options Result;
	int ArgCount = 0;
	LPWSTR* Arguments = CommandLineToArgvW(GetCommandLineW(), &ArgCount);
	if (!Arguments)
		return Result;

	for (int i = 1; i < ArgCount; ++i) {
		const std::wstring_view Argument = Arguments[i];
		auto PathValue = [&](fs::path& Out) {
			if (i + 1 < ArgCount)
				Out = Arguments[++i];
		};
		auto TextValue = [&](std::wstring& Out) {
			if (i + 1 < ArgCount)
				Out = Arguments[++i];
		};

		if (Argument == L"--game")
			PathValue(Result.Game);
		else if (Argument == L"--game-dir")
			PathValue(Result.GameDir);
		else if (Argument == L"--decoy-dir")
			PathValue(Result.DecoyDir);
		else if (Argument == L"--log")
			PathValue(Result.LogPath);
		else if (Argument == L"--parent")
			PathValue(Result.ParentExe);
		else if (Argument == L"--no-official")
			Result.NoOfficial = true;
		else if (Argument == L"--pick")
			Result.Pick = true;
		else if (Argument == L"--debug")
			Result.Debug = true;
		else if (Argument == L"--install")
			Result.Install = true;
		else if (Argument == L"--decoy-source")
			TextValue(Result.DecoySource);
		else if (Argument == L"--args")
			TextValue(Result.GameArgs);
		else if (Argument == L"--help" || Argument == L"-h" || Argument == L"/?")
			Result.Help = true;
		else if (Argument == L"--") {
			// Everything after "--" goes to the game, re-quoted where needed.
			for (++i; i < ArgCount; ++i) {
				std::wstring Token = Arguments[i];
				if (Token.find_first_of(L" \t") != std::wstring::npos)
					Token = L"\"" + Token + L"\"";
				if (!Result.GameArgs.empty())
					Result.GameArgs += L' ';
				Result.GameArgs += Token;
			}
		}
	}

	LocalFree(Arguments);
	return Result;
}
