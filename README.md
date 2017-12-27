# Retroware
This repository contains the code of some ancient system tools that I have written in my spare time, or when I was working for one of various old companies (I have received all the permissions to pubblish them).

They have been almost created for <i><u>studying</u></i> purposes, but they all work pretty good.
Here is the list of included tools:
<ul>
  <li>x86 Memory Bootkit<br>
    A tool that enables Windows Vista/7/8/8.1 32-bit systems to use all the available physical memory (up to 64GB), thus removing the 4GB compatibility limitations. This tool include "Bootkits" technologies (compatible both with UEFI and BIOS) and a fully fledged Installer. Use it only for studying purposes. 
    The first UEFI bootkit was written by me in the year 2012, and this code demonstrate that a company (Quarkslab) has stolen the project in the year 2013 without even mention me. By the way this was another story.<br><br>
    Full analysis link: <a href="https://news.saferbytes.it/analisi/2013/02/saferbytes-x86-memory-bootkit-new-updated-build-is-out/">https://news.saferbytes.it/analisi/2013/02/saferbytes-x86-memory-bootkit-new-updated-build-is-out/</a>.<br><br>
    The project has been abandoned in the fall of year 2013.<br>
    Thanks to Marco Giuliani (<a href="https://twitter.com/eraserhw">@eraserhw</a>) for allowing me the pubblication.<br><br>
  </li>
  
  <li>Windows 8 AppContainers<br>
    This tool was designed to study the AppContainers architecture of the new (at that time) Windows Operating System and was <b>ONLY</b> a draft. It includes an Appcontainer command-line launcher app, a Test Application, and a tool that peek inside some of the Windows 8 security features.<br><br>
    Full analysis link: <a href="https://news.saferbytes.it/analisi/2013/07/securing-microsoft-windows-8-appcontainers/">https://news.saferbytes.it/analisi/2013/07/securing-microsoft-windows-8-appcontainers/</a>.<br><br>
    The project has been abandoned in the fall of year 2013 (after the complete analysis paper has been delivered).<br>
    This project has been developed while working in SaferBytes. Again, thanks to Marco Giuliani (<a href="https://twitter.com/eraserhw">@eraserhw</a>) for allowing me the pubblication.<br><br>
  </li>
  
  <li>PeRebuilder<br>
    This application is the only one that is actually still supported in the present days.
    It is a tool that is able to help malware researches that always dump a complete PE file (Portable Executable) from memory using a debugger (like WinDbg, and NOT Ollydbg :-), just kidding right now...). The tool is able to automatically reconstruct the PE, and save it in a form that the Windows Loader is able to run (fixing the file alignment, relocating each section, fixing the Import address table, and so on...)<br>
  If you are a security enthusiast or a malware anlyst give it a try and, if you need some support or you have some new ideas, feel free to reach me at <a href="mailto:info@andrea-allievi.com">info@andrea-allievi.com</a>.
  </li>
</ul>

