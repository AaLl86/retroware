@echo off
echo This script will create RamBootkit 
echo.
set file1="RamBootkit - Starter.bin"
set file2="RamBootkit - Main.bin"
set file3="Seven Mbr.bin"
set out=Rambootkit.bin

if not exist %file1% goto Error
if not exist %file2% goto Error
copy /b %file1% + %file2% + %file3% %out%
echo.
echo Success!
echo.
goto END

:Error
echo Error! Unable to open source files...
echo Are you sure that you have compiled it???
echo.
pause

:End