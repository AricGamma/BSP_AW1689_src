@echo off
setlocal enabledelayedexpansion
cd /d %~dp0

REM  --> Check for permissions  
"%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system" >nul 2>&1 
  
REM -->we do not have root promission if errorlevel sets
if '%errorlevel%' NEQ '0' (  
    echo Requesting privileges...  
    goto UACReq  
) else ( goto gotUAC )  
  
:UACReq 
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getuac.vbs"  
    echo UAC.ShellExecute "%~s0", "", "", "runas", 1 >> "%temp%\getuac.vbs"  
  
    "%temp%\getuac.vbs"  
    exit /B  
  
:gotUAC  
    if exist "%temp%\getuac.vbs" ( del "%temp%\getuac.vbs" )  


call "SetBuildEnv.cmd"

cd "%WorkRoot%"

REM For building binary files.
if not exist "%WorkRoot%\src\drivers" (
	call "%BUILD_SCRIPTS_ROOT%\PackageFfu.cmd"
	goto :Finish
)

:Select
cls
cd %WorkRoot%

color 0b
title Build - %Chipset%_%DeviceName%_%OemInputTargets%.ffu

echo windows 10 IOT build system for Allwinner Platform
echo.
echo -----------------------------------------------------------------
echo Build Image Name:%Chipset%_%DeviceName%_%OemInputTargets%.ffu
echo -----------------------------------------------------------------
echo.
echo You have following options:
echo ******************************************
echo * 1. Build All Components and pack ffu   *									
echo * 2. Build Windows Drivers               *
echo * 3. Build Bootloader(boot0/uefi)        *
echo * 4. Build Appx                          *
echo * 5. Pack  Bootloader(boot0/uefi)        *
echo * 6. Pack  DevcieConfig packages         *
echo * 7. Pack  Boot-resource package         *
echo * 8. Pack Appx                           *
echo * 9. Pack  FFU                           *
echo * 10. Quit                                *
echo *                                        *
echo ******************************************

choice /c 1234567890 /N /M "Please input your choice:"

echo "Your choice is %errorlevel%"
if errorlevel 10 (
   echo "finish the build"
 	goto :Finish
)


if errorlevel 9 (
  echo "pack ffu"
	call "%BUILD_SCRIPTS_ROOT%\PackFfu.cmd"
	pause	
	goto :Select
)

if errorlevel 8 (
  echo "pack Appx"
	call "%BUILD_SCRIPTS_ROOT%\PackAppx.cmd"
	pause	
	goto :Select
)

if errorlevel 7 (
  echo "pack boot-resource"
	call "%BUILD_SCRIPTS_ROOT%\PackBootresource.cmd"
	pause	
	goto :Select
)

if errorlevel 6 (
	echo "pack device configs"
	call "%BUILD_SCRIPTS_ROOT%\PackDeviceConfigs.cmd"
	pause	
	goto :Select
)

if errorlevel 5 (
	echo "pack bootloader"	
	call "%BUILD_SCRIPTS_ROOT%\PackBootloader.cmd"
	pause	
	goto :Select
)

if errorlevel 4 (
  echo "build appxs"
	call "%BUILD_SCRIPTS_ROOT%\BuildAllAppx.cmd"
	pause	
	goto :Select
)

if errorlevel 3 (
  echo "build bootloader"
	call "%BUILD_SCRIPTS_ROOT%\BuildBootloader.cmd"
	pause	
	goto :Select
)

if errorlevel 2 (
  echo "build windows drivers"
	call "%BUILD_SCRIPTS_ROOT%\BuildWindowsDrivers.cmd"
	echo "finish build windows drivers"
	pause	
	goto :Select
)

if errorlevel 1 (
  echo "build all the component"
	call "%BUILD_SCRIPTS_ROOT%\BuildAllComponents.cmd"
	pause	
	goto :Select
)

:Finish
cd %workroot%
