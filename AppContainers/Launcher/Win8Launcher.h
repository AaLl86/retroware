#pragma once

typedef unsigned long NTSTATUS;
#define STATUS_BUFFER_TOO_SMALL 0x0C0000023
#define STATUS_SUCCESS 0
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define NT_ERROR(Status) ((((ULONG)(Status)) >> 30) == 3)

/*
typedef struct _LOWBOX_DATA {
	HANDLE hAppContainerDir;		// + 0x00 - Handle to "\Sessions\<ID>\AppContainerNamedObjects\<AppContSid>" Directory
	HANDLE hAppContainerRpcDir;		// + 0x08 - Handle to "RPC Control" AppContainer directory
	HANDLE hLocalSymLink;			// + 0x10 - Handle to "Local" AppContainer Symbolic link object
	HANDLE hGlobalSymLink;			// + 0x18 - Handle to "Global" AppContainer Symbolic link object
	HANDLE hSessionSymLink;			// + 0x20 - Handle to "Session" AppContainer Symbolic link object
	HANDLE hAppContNamedPipe;		// + 0x28 - Handle to this App Container named pipe
} LOWBOX_DATA, *PLOWBOX_DATA;

NTSTATUS ZwCreateLowBoxToken(HANDLE * phLowBoxToken, HANDLE hOrgToken, ACCESS_MASK DesiredAccess, 
	OBJECT_ATTRIBUTES * pOa, PSID pAppContainerSid, DWORD capabilityCount, 
	PSID_AND_ATTRIBUTES capabilities, DWORD lowBoxStructHandleCount, PLOWBOX_DATA lowBoxStruct);
*/

/* Flags to indicate if an attribute is present in the list. 
typedef enum _PROC_THREAD_ATTRIBUTE_NUM {
	ProcThreadAttributeParentProcess = 0,
	ProcThreadAttributeExtendedFlags = 1,
	ProcThreadAttributeHandleList = 2,
	ProcThreadAttributeGroupAffinity = 3,
	ProcThreadAttributePreferredNode = 4,
	ProcThreadAttributeIdealProcessor = 5,
	ProcThreadAttributeUmsThread = 6,
	ProcThreadAttributeMitigationPolicy = 7,
	ProcThreadAttributePackageFullName = 8,
	ProcThreadAttributeSecurityCapabilities = 9,
	ProcThreadAttributeConsoleReference = 10,
	ProcThreadAttributeMax = 11
} PROC_THREAD_ATTRIBUTE_NUM, *PPROC_THREAD_ATTRIBUTE_NUM ;
*/
#define ProcThreadAttributePackageFullName 8

// I believe these are used so that Windows doesn't have to scan the complete list to see if an attribute is present.
#define PARENT_PROCESS    (1 << ProcThreadAttributeParentProcess)                                                    
#define EXTENDED_FLAGS    (1 << ProcThreadAttributeExtendedFlags)                                                    
#define HANDLE_LIST       (1 << ProcThreadAttributeHandleList)                                                       
#define GROUP_AFFINITY    (1 << ProcThreadAttributeGroupAffinity)                                                    
#define PREFERRED_NODE    (1 << ProcThreadAttributePreferredNode)                                                    
#define IDEAL_PROCESSOR   (1 << ProcThreadAttributeIdealProcessor)                                                   
#define UMS_THREAD        (1 << ProcThreadAttributeUmsThread)                                                        
#define MITIGATION_POLICY (1 << ProcThreadAttributeMitigationPolicy) 
#define PACKAGE_FULL_NAME (1 << ProcThreadAttributePackageFullName)
//#define SECURITY_CAPABILITIES (1 << ProcThreadAttributeSecurityCapabilities)

/*
#define PROC_THREAD_ATTRIBUTE_NUMBER    0x0000FFFF
#define PROC_THREAD_ATTRIBUTE_THREAD    0x00010000  // Attribute may be used with thread creation
#define PROC_THREAD_ATTRIBUTE_INPUT     0x00020000  // Attribute is input only
#define PROC_THREAD_ATTRIBUTE_ADDITIVE  0x00040000  // Attribute may be "accumulated," e.g. bitmasks, counters, etc.

#define ProcThreadAttributeValue(Number, Thread, Input, Additive) \
    (((Number) & PROC_THREAD_ATTRIBUTE_NUMBER) | \
     ((Thread != FALSE) ? PROC_THREAD_ATTRIBUTE_THREAD : 0) | \
     ((Input != FALSE) ? PROC_THREAD_ATTRIBUTE_INPUT : 0) | \
     ((Additive != FALSE) ? PROC_THREAD_ATTRIBUTE_ADDITIVE : 0))

#define PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES \
    ProcThreadAttributeValue (ProcThreadAttributeSecurityCapabilities, FALSE, TRUE, FALSE)

#define PROC_THREAD_ATTRIBUTE_GROUP_AFFINITY \												// Value 0x00030003
	ProcThreadAttributeValue(ProcThreadAttributeGroupAffinity, TRUE, TRUE, FALSE)
*/
#define PROC_THREAD_ATTRIBUTE_IDEAL_PROCESSOR_FAKE \
	ProcThreadAttributeValue(ProcThreadAttributeIdealProcessor, FALSE, TRUE, FALSE)

#define PROC_THREAD_ATTRIBUTE_PACKAGE_FULL_NAME \
    ProcThreadAttributeValue (ProcThreadAttributePackageFullName, FALSE, TRUE, FALSE)


// This structure stores the value for each attribute 
struct PROC_THREAD_ATTRIBUTE_ENTRY 
{ 
     DWORD_PTR   Attribute;					// + 0x00 - PROC_THREAD_ATTRIBUTE_xxx 
     SIZE_T      cbSize;					// + 0x08
     PVOID       lpValue;					// + 0x10
};

// This structure contains a list of attributes that have been added using UpdateProcThreadAttribute
struct PROC_THREAD_ATTRIBUTE_LIST
{
	DWORD                          dwFlags;						// + 0x00
	ULONG                          Size;						// + 0x04
	ULONG                          Count;						// + 0x08
	ULONG                          Reserved;					// + 0x0c
	PULONG                         Unknown;						// + 0x10
	PROC_THREAD_ATTRIBUTE_ENTRY    Entries[ANYSIZE_ARRAY];		// + 0x18
};

struct PROCESS_ATTRIBUTE_LIST {
	DWORD dwSize;
	DWORD uCount;



};


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
  TokenAppContainerSid,
  TokenAppContainerNumber,
  TokenUserClaimAttributes,
  TokenDeviceClaimAttributes,
  TokenRestrictedUserClaimAttributes,		// 35
  TokenRestrictedDeviceClaimAttributes,		// 36
  TokenDeviceGroups,
  TokenRestrictedDeviceGroups,
  TokenSecurityAttributes,
  TokenIsRestricted,						// 40
  MaxTokenInfoClass
} TOKEN_INFORMATION_CLASS, *PTOKEN_INFORMATION_CLASS;
*/

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
    PVOID SecurityDescriptor;			// + 0x20 Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;		// + 0x28 Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

extern "C" NTSTATUS ZwSetInformationToken( HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass,
	PVOID TokenInformation, ULONG TokenInformationLength);

extern "C" NTSTATUS ZwQuerySecurityObject(HANDLE Handle, SECURITY_INFORMATION SecurityInformation, 
	PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG Length, PULONG LengthNeeded);

extern "C" NTSTATUS NTAPI ZwQueryInformationToken(HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass,
	PVOID TokenInformation, ULONG TokenInformationLength, PULONG ReturnLength);

// Data returned by TokenCapabilities query
typedef struct _TOKEN_CAPABILITIES {
	DWORD CapabilityCount;									// + 0x00		
	SID_AND_ATTRIBUTES Capabilities[ANYSIZE_ARRAY];			// + 0x08
} TOKEN_CAPABILITIES, * PTOKEN_CAPABILITIES;

// Test Windows 8 Appcontainer Tokens
bool TestAppContainer();

// Enable a privilege in a token
bool EnablePrivilege(HANDLE hToken, LPTSTR privName); 

// Add AppContainer ACL to a file or directory
bool AddAppContainerAclToFile(PSID appContSid, LPTSTR fileName);

// General purposes Tests
bool DoSecurityTests();

