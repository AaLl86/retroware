# SCRIPT To enable source level debugging in IDA GDB
# You have to replace "x.base" variable with real base address and
# "x.name" with original binary file path.

x = idaapi.module_info_t()
x.base = 0x10000000
x.name ="e:\\AaLl86 Projects\\Win7x86Bootkit\\UEFI\\Debug\\UefiMemoryBtk.efi"
idaapi.add_virt_module(x)
