#pragma once
#pragma comment (lib, "ntdll.lib")

typedef unsigned long NTSTATUS;
#define STATUS_SUCCESS 0
#define STATUS_BUFFER_TOO_SMALL 0xC0000023
#pragma warning(disable: 4005)
#define NTSYSAPI extern "C" DECLSPEC_IMPORT
#pragma warning(default: 4005)
/* New Windows 8 Token Information Class
typedef enum _TOKEN_INFORMATION_CLASS { 
  TokenUser                             = 1,
  TokenGroups,							// 2
  TokenPrivileges,						// 3
  TokenOwner,							// 4
  TokenPrimaryGroup,					// 5
  TokenDefaultDacl,						// 6
  TokenSource,							// 7
  TokenType,							// 8
  TokenImpersonationLevel,				// 9
  TokenStatistics,						// 10 - 0x0A
  TokenRestrictedSids,					// 11 - 0x0B
  TokenSessionId,						// 12 - 0x0C
  TokenGroupsAndPrivileges,				// 13 - 0x0D
  TokenSessionReference,				// 14 - 0x0E
  TokenSandBoxInert,					// 15 - 0x0F
  TokenAuditPolicy,						// 16 - 0x10
  TokenOrigin,
  TokenElevationType,
  TokenLinkedToken,
  TokenElevation,							// 20
  TokenHasRestrictions,
  TokenAccessInformation,
  TokenVirtualizationAllowed,
  TokenVirtualizationEnabled,
  TokenIntegrityLevel,						// 25
  TokenUIAccess,							// 26
  TokenMandatoryPolicy,
  TokenLogonSid,
  TokenIsAppContainer,						// 29
  TokenCapabilities,						// 30
  TokenAppContainerSid,						// 31
  TokenAppContainerNumber,					// 32
  TokenUserClaimAttributes,					// 33
  TokenDeviceClaimAttributes,				// 34
  TokenRestrictedUserClaimAttributes,		// 35
  TokenRestrictedDeviceClaimAttributes,		// 36
  TokenDeviceGroups,
  TokenRestrictedDeviceGroups,
  TokenSecurityAttributes,
  TokenIsRestricted,						// 40
  MaxTokenInfoClass
} TOKEN_INFORMATION_CLASS, *PTOKEN_INFORMATION_CLASS;
*/

#define TokenIsAppContainer (TOKEN_INFORMATION_CLASS)29
#define TokenCapabilities (TOKEN_INFORMATION_CLASS)30
#define TokenAppContainerSid (TOKEN_INFORMATION_CLASS)31
#define TokenAppContainerNumber (TOKEN_INFORMATION_CLASS)32

NTSYSAPI NTSTATUS NTAPI ZwQueryInformationToken(HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass,
	PVOID TokenInformation, ULONG TokenInformationLength, PULONG ReturnLength);

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
	PWCH   Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;						// + 0x00
    HANDLE RootDirectory;				// + 0x08
    PUNICODE_STRING ObjectName;			// + 0x10
    ULONG Attributes;					// + 0x18
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _SECURITY_CAPABILITIES {
    PSID AppContainerSid;						// + 0x00
    PSID_AND_ATTRIBUTES Capabilities;			// + 0x08
    DWORD CapabilityCount;						// + 0x10
    DWORD Reserved;								// + 0x14
} SECURITY_CAPABILITIES, *PSECURITY_CAPABILITIES, *LPSECURITY_CAPABILITIES;

// Data returned by TokenCapabilities query
typedef struct _TOKEN_CAPABILITIES {
	DWORD CapabilityCount;									// + 0x00		
	SID_AND_ATTRIBUTES Capabilities[ANYSIZE_ARRAY];			// + 0x08
} TOKEN_CAPABILITIES, * PTOKEN_CAPABILITIES;

// Data returned by TokenAppContainerSid query
typedef struct _TOKEN_APPCONTAINER {
	PSID AppContainerSid;
} TOKEN_APPCONTAINER, * PTOKEN_APPCONTAINER;

bool GetTokenAppContainerState(bool * pbIsAppContainer, HANDLE hToken = NULL, bool bDisplayData = true);
