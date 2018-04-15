# Talos Hypervisor 0.1

This project is an Hypervisor skeleton. It is compatible with Intel Vt-x.
It does nothing, except to put all the CPUs in a TALOS Hypervisor. 
There are a lot of difficulties while developing an Hypervior, and a Virtual Machine Monitor.
This code has the goal to be a start point for a real Virtual Machine Monitor or for an hypervisor.

Whith an Hypervisor you can monitor some particular instructions or memory regions, you can implement an invisible debugger, and a lot of other useful things.
Here is the list of things that can be improved or implement: <br><ol type="1">
<li> To proper virtualize all the processors my code makes use of the System Threads. You can use even the DPC - Deferred Procedure Calls, which is better because they automatically raise the IRQL to 2 - Dispatch Level</li>
<br>
<li> My code doesn't implement an EPT - Extende Page Tables. With them you can map the real hardware physical memory and create some memory regions that are invisible to the guest OS<br>
   Implementing the EPT could be very interesting.<br>
   Here are a list of references that you can use to do that:<br>
   <ol type="a">
   <li>Intel System Programming Guide Manual, volume 3, chapter 28 - VMX SUPPORT FOR ADDRESS TRANSLATION</li>
   <li>http://forum.osdev.org/viewtopic.php?f=1&t=28228&p=237502&hilit=ept#p237502</li>
   <li>https://github.com/JulesWang/JOS-vmx</li></ol>
<br>

<li> My code doesn't produce any VM Exits except for the basic instructions (CPUID, VMCALL and CR registers access, see the code inside "VmxMain.cpp" for more information).
   The initialization code ("VmSetupVmcs" routine) located inside the "VmxInit.h" file, configures all the Exit Control fields in the target VMCS as FALSE. 
   A well-designed hypervisor configures a lot of VM-Exits.</li> 
<br>

<li>My hypervisor doesn't make use of the  "Unrestricted Guest" flag (bit 7 in Secondary Exit Controls), that would allow a guest to start in real mode.
   A real VM needs to start the processor in Real Mode (UEFI Bioses do this automatically)</li>
</ol>
<br>

I have developed a test application that emit a CPUID instruction to proper check if my test hypervisor is up and run.
To proper run the kernel mode driver in a production environment, you have to sign the driver, or disable the Windows Driver Signing Enforcement.

Last release: 02/15/2016
