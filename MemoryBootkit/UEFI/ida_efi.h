// AaLl86 EFI Data structures Declarations for IDA

typedef void VOID, *LPVOID;
typedef VOID *EFI_HANDLE;

typedef enum {
  EfiReservedMemoryType,
  EfiLoaderCode,
  EfiLoaderData,
  EfiBootServicesCode,
  EfiBootServicesData,
  EfiRuntimeServicesCode,
  EfiRuntimeServicesData,
  EfiConventionalMemory,
  EfiUnusableMemory,
  EfiACPIReclaimMemory,
  EfiACPIMemoryNVS,
  EfiMemoryMappedIO,
  EfiMemoryMappedIOPortSpace,
  EfiPalCode,
  EfiMaxMemoryType
} EFI_MEMORY_TYPE;

//
// EFI Data Types based on ANSI C integer types in EfiBind.h
//
typedef unsigned char BOOLEAN;
typedef int INTN;
typedef unsigned int UINTN;
typedef char INT8;
typedef unsigned char UINT8;
typedef short INT16;
typedef unsigned short UINT16;
typedef int INT32;
typedef unsigned int UINT32;
typedef __int64 INT64;
typedef unsigned __int64 UINT64;
typedef unsigned char CHAR8;
typedef unsigned short CHAR16;
typedef UINT64 EFI_LBA;
typedef UINTN EFI_STATUS;

typedef struct {
  UINT64  Signature;								// +0x00
  UINT32  Revision;									// +0x08
  UINT32  HeaderSize;								// +0x0c
  UINT32  CRC32;										// +0x10
  UINT32  Reserved;									// +0x14
} EFI_TABLE_HEADER;

typedef struct {
  UINT32  Data1;
  UINT16  Data2;
  UINT16  Data3;
  UINT8   Data4[8];
} EFI_GUID;

//
// EFI Configuration Table
//
typedef struct {
  EFI_GUID  VendorGuid;												// + 0x00
  VOID      *VendorTable;											// + 0x10
} EFI_CONFIGURATION_TABLE;


typedef struct {
  EFI_TABLE_HEADER                            Hdr;

  //
  // Task priority functions
  //
  LPVOID            		                   RaiseTPL;
  LPVOID                             RestoreTPL;

  //
  // Memory functions
  //
  LPVOID                          AllocatePages;
  LPVOID                              FreePages;
  LPVOID                          GetMemoryMap;
  LPVOID                           AllocatePool;
  LPVOID                               FreePool;

  //
  // Event & timer functions
  //
  LPVOID                            CreateEvent;
  LPVOID                               SetTimer;
  LPVOID                          WaitForEvent;
  LPVOID                            SignalEvent;
  LPVOID                             CloseEvent;
  LPVOID                             CheckEvent;

  //
  // Protocol handler functions
  //
  LPVOID              InstallProtocolInterface;
  LPVOID            ReinstallProtocolInterface;
  LPVOID            UninstallProtocolInterface;
  LPVOID                         HandleProtocol;
  VOID                                        *Reserved;
  LPVOID                RegisterProtocolNotify;
  LPVOID                           LocateHandle;
  LPVOID                      LocateDevicePath;
  LPVOID             InstallConfigurationTable;

  //
  // Image functions
  //
  LPVOID                              LoadImage;
  LPVOID                             StartImage;
  LPVOID                                    Exit;
  LPVOID                            UnloadImage;
  LPVOID                      ExitBootServices;

  //
  // Misc functions
  //
  LPVOID                GetNextMonotonicCount;
  LPVOID                                   Stall;
  LPVOID                      SetWatchdogTimer;

  //
  // ////////////////////////////////////////////////////
  // EFI 1.1 Services
    //////////////////////////////////////////////////////
  //
  // DriverSupport Services
  //
  LPVOID                      ConnectController;
  LPVOID                   DisconnectController;

  //
  // Added Open and Close protocol for the new driver model
  //
  LPVOID                           OpenProtocol;
  LPVOID                          CloseProtocol;
  LPVOID               OpenProtocolInformation;

  //
  // Added new services to EFI 1.1 as Lib to reduce code size.
  //
  LPVOID                    ProtocolsPerHandle;
  LPVOID                    LocateHandleBuffer;
  LPVOID                         LocateProtocol;

  LPVOID    InstallMultipleProtocolInterfaces;
  LPVOID  UninstallMultipleProtocolInterfaces;

  //
  // CRC32 services
  //
  LPVOID                         CalculateCrc32;

  //
  // Memory Utility Services
  //
  LPVOID                                CopyMem;
  LPVOID                                 SetMem;

  //
  // UEFI 2.0 Extension to the table
  //
  LPVOID                         CreateEventEx;

} EFI_BOOT_SERVICES;


typedef struct _EFI_SYSTEM_TABLE {
  EFI_TABLE_HEADER              Hdr;														// + 0x00
  CHAR16                        *FirmwareVendor;								// + 0x18
  UINT32                        FirmwareRevision;								// + 0x1c
  EFI_HANDLE                    ConsoleInHandle;								// + 0x20
  LPVOID  											*ConIn;													// + 0x24
  EFI_HANDLE                    ConsoleOutHandle;								// + 0x28
  LPVOID                    		*ConOut;												// + 0x2c
  EFI_HANDLE                    StandardErrorHandle;						// + 0x30
  LPVOID                      	*StdErr;												// + 0x34
  LPVOID                        *RuntimeServices;								// + 0x38
  EFI_BOOT_SERVICES             *BootServices;									// + 0x3c
  UINTN                         NumberOfTableEntries;						// + 0x40
  EFI_CONFIGURATION_TABLE       *ConfigurationTable;						// + 0x44
} EFI_SYSTEM_TABLE; 


//
// EFI_SYSTEM_TABLE & EFI_IMAGE_UNLOAD are defined in EfiApi.h
//
#define EFI_LOADED_IMAGE_INFORMATION_REVISION 0x1000

typedef struct {
  UINT8 Type;
  UINT8 SubType;
  UINT8 Length[2];
} EFI_DEVICE_PATH_PROTOCOL;

typedef struct {
  UINT32                    Revision;
  EFI_HANDLE                ParentHandle;
  EFI_SYSTEM_TABLE          *SystemTable;

  //
  // Source location of image
  //
  EFI_HANDLE                DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL  *FilePath;
  VOID                      *Reserved;

  //
  // Images load options
  //
  UINT32                    LoadOptionsSize;
  VOID                      *LoadOptions;

  //
  // Location of where image was loaded
  //
  VOID                      *ImageBase;
  UINT64                    ImageSize;
  EFI_MEMORY_TYPE           ImageCodeType;
  EFI_MEMORY_TYPE           ImageDataType;

  //
  // If the driver image supports a dynamic unload request
  //
  LPVOID          Unload;

} EFI_LOADED_IMAGE_PROTOCOL;