#include "stdafx.h"
#include "SignatureCheck.h"
#include "Core.h"
#include "DebugLog.h"

bool SignatureCheck::HasValidSignature(const fs::path& File)
{
	WINTRUST_FILE_INFO FileInfo{};
	FileInfo.cbStruct = sizeof(FileInfo);
	FileInfo.pcwszFilePath = File.c_str();

	WINTRUST_DATA Data{};
	Data.cbStruct = sizeof(Data);
	Data.dwUIChoice = WTD_UI_NONE;
	Data.fdwRevocationChecks = WTD_REVOKE_NONE;
	Data.dwUnionChoice = WTD_CHOICE_FILE;
	Data.pFile = &FileInfo;
	Data.dwStateAction = WTD_STATEACTION_VERIFY;
	Data.dwProvFlags = WTD_REVOCATION_CHECK_NONE;

	GUID Action = WINTRUST_ACTION_GENERIC_VERIFY_V2;
	const LONG Result = WinVerifyTrust(nullptr, &Action, &Data);

	Data.dwStateAction = WTD_STATEACTION_CLOSE;
	WinVerifyTrust(nullptr, &Action, &Data);
	return Result == 0;
}

std::wstring SignatureCheck::SignerSubject(const fs::path& File, std::wstring* Thumbprint)
{
	HCERTSTORE Store = nullptr;
	HCRYPTMSG Message = nullptr;
	std::wstring Subject;

	if (CryptQueryObject(CERT_QUERY_OBJECT_FILE, File.c_str(), CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
		CERT_QUERY_FORMAT_FLAG_BINARY, 0, nullptr, nullptr, nullptr, &Store, &Message, nullptr)) {
		DWORD Size = 0;
		if (CryptMsgGetParam(Message, CMSG_SIGNER_INFO_PARAM, 0, nullptr, &Size)) {
			std::vector<BYTE> Buffer(Size);
			auto* Signer = reinterpret_cast<CMSG_SIGNER_INFO*>(Buffer.data());
			if (CryptMsgGetParam(Message, CMSG_SIGNER_INFO_PARAM, 0, Signer, &Size)) {
				CERT_INFO Info{};
				Info.Issuer = Signer->Issuer;
				Info.SerialNumber = Signer->SerialNumber;
				PCCERT_CONTEXT Certificate = CertFindCertificateInStore(
					Store, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_SUBJECT_CERT, &Info, nullptr);
				if (Certificate) {
					const DWORD Length = CertNameToStrW(Certificate->dwCertEncodingType,
						&Certificate->pCertInfo->Subject, CERT_X500_NAME_STR, nullptr, 0);
					if (Length > 1) {
						Subject.resize(Length);
						CertNameToStrW(Certificate->dwCertEncodingType, &Certificate->pCertInfo->Subject,
							CERT_X500_NAME_STR, Subject.data(), Length);
						Subject.resize(static_cast<size_t>(Length) - 1);
					}
					if (Thumbprint) {
						BYTE Hash[20]{};
						DWORD HashSize = sizeof(Hash);
						if (CertGetCertificateContextProperty(Certificate, CERT_HASH_PROP_ID, Hash, &HashSize)) {
							static constexpr wchar_t HexDigits[] = L"0123456789ABCDEF";
							Thumbprint->clear();
							for (DWORD i = 0; i < HashSize; ++i) {
								Thumbprint->push_back(HexDigits[Hash[i] >> 4]);
								Thumbprint->push_back(HexDigits[Hash[i] & 0xF]);
							}
						}
					}
					CertFreeCertificateContext(Certificate);
				}
			}
		}
	}

	if (Message)
		CryptMsgClose(Message);
	if (Store)
		CertCloseStore(Store, 0);
	return Subject;
}

bool SignatureCheck::IsTrustedFile(const fs::path& File)
{
	std::error_code Error;
	if (!fs::is_regular_file(File, Error)) {
		DebugLog::Write(L"  not a file: " + File.wstring());
		return false;
	}
	if (!HasValidSignature(File)) {
		DebugLog::Write(L"  no valid Authenticode signature: " + File.wstring());
		return false;
	}

	std::wstring Thumbprint;
	const std::wstring Subject = SignerSubject(File, &Thumbprint);
	DebugLog::Write(L"  signer: " + Subject + L" | SHA1 " + Thumbprint);
	for (const std::wstring_view Publisher : TrustedPublishers) {
		if (Core::IContains(Subject, Publisher))
			return true;
	}
	DebugLog::Write(L"  signer is not a Hypergryph entity");
	return false;
}
