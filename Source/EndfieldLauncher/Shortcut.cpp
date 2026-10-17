#include "stdafx.h"
#include "Shortcut.h"
#include "Core.h"
#include "GameLocator.h"

namespace {

// Releases a COM interface in a std::unique_ptr.
class ComReleaser {
public:
	void operator()(IUnknown* Object) const noexcept
	{
		if (Object)
			Object->Release();
	}
};

template <class Interface>
using ComPtr = std::unique_ptr<Interface, ComReleaser>;

// Frees a CoTaskMemAlloc'd string, such as SHGetKnownFolderPath returns.
class TaskMemFree {
public:
	void operator()(wchar_t* Text) const noexcept
	{
		CoTaskMemFree(Text);
	}
};

// Calls CoUninitialize on destruction when this object's CoInitializeEx succeeded. If the thread
// already runs in another apartment (RPC_E_CHANGED_MODE) COM is usable as is and is left alone.
class ComScope {
public:
	ComScope() : Result(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)) {}
	~ComScope()
	{
		if (SUCCEEDED(Result))
			CoUninitialize();
	}
	ComScope(const ComScope&) = delete;
	ComScope& operator=(const ComScope&) = delete;

	bool Usable() const noexcept { return SUCCEEDED(Result) || Result == RPC_E_CHANGED_MODE; }

private:
	HRESULT Result;
};

std::wstring HResultText(HRESULT Result)
{
	return Core::LastErrorText(static_cast<DWORD>(HRESULT_CODE(Result)));
}

// <desktop>\Endfield Launcher.lnk. The desktop is the current user's, redirection included.
bool DesktopLinkPath(fs::path& LinkPath, std::wstring& Failure)
{
	PWSTR RawDesktop = nullptr;
	const HRESULT Result = SHGetKnownFolderPath(FOLDERID_Desktop, KF_FLAG_DEFAULT, nullptr, &RawDesktop);
	const std::unique_ptr<wchar_t, TaskMemFree> Desktop(RawDesktop);
	if (FAILED(Result) || !Desktop) {
		Failure = L"Can't find the desktop folder:\n" + HResultText(Result);
		return false;
	}
	LinkPath = fs::path(Desktop.get()) / Shortcut::FileName;
	return true;
}

// Same file: identical text ignoring case, or the same file on disk (8.3 names, links, "..").
bool SamePath(const fs::path& Left, const fs::path& Right)
{
	if (Core::IEquals(Left.lexically_normal().wstring(), Right.lexically_normal().wstring()))
		return true;
	std::error_code Error;
	return fs::equivalent(Left, Right, Error) && !Error;
}

} // namespace

bool Shortcut::WasPromptedFor(const fs::path& Exe)
{
	if (Exe.empty())
		return false;
	wchar_t Stored[MAX_PATH * 2]{};
	DWORD Size = sizeof(Stored);
	if (RegGetValueW(HKEY_CURRENT_USER, GameLocator::RegistryKey, PromptedValue, RRF_RT_REG_SZ, nullptr, Stored, &Size) != ERROR_SUCCESS)
		return false;
	return Stored[0] != L'\0' && SamePath(Stored, Exe);
}

void Shortcut::MarkPrompted(const fs::path& Exe)
{
	if (Exe.empty())
		return;
	HKEY Key = nullptr;
	if (RegCreateKeyExW(HKEY_CURRENT_USER, GameLocator::RegistryKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &Key, nullptr) != ERROR_SUCCESS)
		return;
	const std::wstring Text = Exe.wstring();
	RegSetValueExW(Key, PromptedValue, 0, REG_SZ, reinterpret_cast<const BYTE*>(Text.c_str()),
		static_cast<DWORD>((Text.size() + 1) * sizeof(wchar_t)));
	RegCloseKey(Key);
}

Shortcut::State Shortcut::Inspect(const fs::path& Target)
{
	std::wstring Ignored;
	fs::path LinkPath;
	if (!DesktopLinkPath(LinkPath, Ignored))
		return State::Missing;

	std::error_code Error;
	if (!fs::exists(LinkPath, Error))
		return State::Missing;

	// Something is there; from here on, anything unreadable counts as pointing elsewhere.
	if (Target.empty())
		return State::Different;

	const ComScope Com;
	if (!Com.Usable())
		return State::Different;

	IShellLinkW* RawLink = nullptr;
	HRESULT Result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&RawLink));
	const ComPtr<IShellLinkW> Link(RawLink);
	if (FAILED(Result) || !Link)
		return State::Different;

	IPersistFile* RawFile = nullptr;
	Result = Link->QueryInterface(IID_PPV_ARGS(&RawFile));
	const ComPtr<IPersistFile> File(RawFile);
	if (FAILED(Result) || !File)
		return State::Different;

	// Load only reads the file. Resolve is not called: it can search the disk and show UI.
	if (FAILED(File->Load(LinkPath.c_str(), STGM_READ)))
		return State::Different;

	wchar_t Stored[MAX_PATH * 2]{};
	if (FAILED(Link->GetPath(Stored, static_cast<int>(sizeof(Stored) / sizeof(Stored[0])), nullptr, 0)) || Stored[0] == L'\0')
		return State::Different;

	return SamePath(Stored, Target) ? State::Matches : State::Different;
}

bool Shortcut::CreateOnDesktop(const fs::path& Target, const std::wstring& Arguments, std::wstring& Failure)
{
	if (Target.empty()) {
		Failure = L"Can't find this exe's own path.";
		return false;
	}

	const ComScope Com;
	if (!Com.Usable()) {
		Failure = L"Can't start COM to create the shortcut.";
		return false;
	}

	fs::path LinkPath;
	if (!DesktopLinkPath(LinkPath, Failure))
		return false;

	IShellLinkW* RawLink = nullptr;
	HRESULT Result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&RawLink));
	const ComPtr<IShellLinkW> Link(RawLink);
	if (FAILED(Result) || !Link) {
		Failure = L"Can't create the shortcut object:\n" + HResultText(Result);
		return false;
	}

	Link->SetPath(Target.c_str());
	Link->SetArguments(Arguments.c_str());
	Link->SetWorkingDirectory(Target.parent_path().c_str());
	Link->SetIconLocation(Target.c_str(), 0);
	Link->SetDescription(Description);

	IPersistFile* RawFile = nullptr;
	Result = Link->QueryInterface(IID_PPV_ARGS(&RawFile));
	const ComPtr<IPersistFile> File(RawFile);
	if (FAILED(Result) || !File) {
		Failure = L"Can't save the shortcut:\n" + HResultText(Result);
		return false;
	}

	// IPersistFile::Save replaces an existing file, but not a read-only one. Clear the attribute
	// and delete the old shortcut first; if that fails, Save reports the error.
	SetFileAttributesW(LinkPath.c_str(), FILE_ATTRIBUTE_NORMAL);
	DeleteFileW(LinkPath.c_str());

	Result = File->Save(LinkPath.c_str(), TRUE);
	if (FAILED(Result)) {
		Failure = L"Can't write the shortcut:\n" + LinkPath.wstring() + L"\n\n" + HResultText(Result);
		return false;
	}
	return true;
}
