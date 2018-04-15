Talos Hypervisor 0.1

This project is an Hypervisor skeleton. It is compatible with Intel Vt-x.
It does nothing, except to put all the CPUs in a TALOS Hypervisor. 
There are a lot of difficulties while developing an Hypervior, and a Virtual Machine Monitor.
This code has the goal to be a start point for a real Virtual Machine Monitor or for an hypervisor.

Whith an Hypervisor you can monitor some particular instructions or memory regions, you can implement an invisible debugger, and a lot of other useful things.
Here is the list of things that can be improved or implement:
1. To proper virtualize all the processors my code makes use of the System Threads. You can use even the DPC - Deferred Procedure Calls, which is better because they automatically raise the IRQL to 2 - Dispatch Level
2. My code doesn't implement an EPT - Extende Page Tables. With them you can map the real hardware physical memory and create some memory regions that are invisible to the guest OS
   Implementing the EPT could be very interesting.
   Here are a list of references that you can use to do that:
   a. Intel System Programming Guide Manual, volume 3, chapter 28 - VMX SUPPORT FOR ADDRESS TRANSLATION
   b. http://forum.osdev.org/viewtopic.php?f=1&t=28228&p=237502&hilit=ept#p237502
   c. https://github.com/JulesWang/JOS-vmx

3. My code doesn't produce any VM Exits except for the basic instructions (CPUID, VMCALL and CR registers access, see the code inside "VmxMain.cpp" for more information).
   The initialization code ("VmSetupVmcs" routine) located inside the "VmxInit.h" file, configures all the Exit Control fields in the target VMCS as FALSE. 
   A well-designed hypervisor configures a lot of VM-Exits. 
   
4. My hypervisor doesn't make use of the  "Unrestricted Guest" flag (bit 7 in Secondary Exit Controls), that would allow a guest to start in real mode.
   A real VM needs to start the processor in Real Mode (UEFI Bioses do this automatically)

I have developed a test application that emit a CPUID instruction to proper check if my test hypervisor is up and run.
To proper run the kernel mode driver in a production environment, you have to sign the driver, or disable the Windows Driver Signing Enforcement.

Last release: 02/15/2016