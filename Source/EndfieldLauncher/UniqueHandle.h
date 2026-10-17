#pragma once

#include <memory>

// Deleter for handles in a std::unique_ptr. Null and INVALID_HANDLE_VALUE both mean "nothing to
// close": CreateProcess leaves the first, CreateToolhelp32Snapshot reports failure with the second.
class HandleCloser {
public:
	void operator()(HANDLE Handle) const noexcept
	{
		if (Handle && Handle != INVALID_HANDLE_VALUE)
			CloseHandle(Handle);
	}
};

using UniqueHandle = std::unique_ptr<void, HandleCloser>;
