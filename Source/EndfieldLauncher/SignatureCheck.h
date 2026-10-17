#pragma once

#include <array>
#include <string>
#include <string_view>

// Decides whether a file can be the game's parent. The signature has to verify and the signer's
// subject has to name a Hypergryph entity. Each step goes to DebugLog.
class SignatureCheck {
public:
	// Substrings of the signer subject that identify Hypergryph.
	static constexpr std::array<std::wstring_view, 3> TrustedPublishers{ L"Hypergryph", L"GRYPHLINE", L"GRYPH FRONTIER" };

	// True for a regular file with a valid signature from a trusted publisher.
	static bool IsTrustedFile(const fs::path& File);

private:
	// Authenticode check without revocation lookup, as the client does.
	static bool HasValidSignature(const fs::path& File);

	// Subject of the signing certificate, empty on failure. Thumbprint, if given, gets the SHA-1
	// as upper-case hex for the log.
	static std::wstring SignerSubject(const fs::path& File, std::wstring* Thumbprint = nullptr);
};
