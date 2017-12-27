AaLl86 x86 Memory Bootkit
Last Revision: 20/02/2013

30/08/2012 - Fixed original Mbr issue on removable devices
26/01/2013 - Added UEFI Support
31/01/2012 - Added VBR Bootkit support
07/02/2013 - Added dynamically inserted USB pendrive list change
20/02/2013 - Added secure boot detection routines

|---------------------------------------------------------------------------------------|
|					Readme File					|
|---------------------------------------------------------------------------------------|

Removable Devices 
	1. Get real Nt Device Name
	2. Get physical Disk Device of Volume Object
		2a. If physical disk exists, use it and enable "bDeleteDosName" flag
		2b. Otherwise Get Volume name and transform it in a GUID Dos Device Name
			2b1. If Dos GUID device doesn't exist use Volume DOS name as device object
	3. If (GetDiskDevType(devObj) >= DISK_TYPE_GUID AND bDeleteDosName) Dismount original volume and delete its drive letter
	4. If (GetDiskDevType(devObj) == DISK_TYPE_MBR) execute MBR Style Removable Installation
	5. Otherwise execute FS Style Removable Installation

- MBR Style Removable Installation
	1. Copy original drive MBR, store it in another sector and install normally in Removable Device, except
	   that use Original MBR reading from original Hard Disk (type 0x81)
	
- FS style Partitioning
	1. Create a Windows 7 MBR sector contains ONE Partition overspreads entire drive
	2. Write new MBR to Sector 0 of removable device
	3. Call DevMbrRemoveBootkit on removable drive, with bRemovable parameter set to TRUE




|---------------------------------------------------------------------------------------|
|				   Developer Notes					|
|---------------------------------------------------------------------------------------|
ORIGINAL SITUATUION
Sector 0: FileSystem VBR

NEW SITUATION
Sector 0: Bootkit Starter and a fake MBR with ONE Partition overspreads entire drive
Sector 0xE: Bootkit code
Sector 0x10: Windows 7 original MBR with new partition

NEW VERSION 1.2
- Add support for Windows activation loader