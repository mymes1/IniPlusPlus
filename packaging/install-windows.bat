@echo off
rem ============================================================================================
rem  Ini++ (Unicode) - installer for Clickteam Fusion 2.5 / Multimedia Fusion 2 (Unicode)
rem
rem  Copies the extension into the folders Fusion loads it from:
rem    <Fusion>\Extensions\Unicode\INI++.mfx       (edittime: used while editing)
rem    <Fusion>\Data\Runtime\Unicode\INI++.mfx     (runtime: used when running/building apps)
rem
rem  Usage:
rem    install.bat                   detect every installed Fusion/MMF2 from the registry
rem    install.bat "D:\Fusion 2.5"   use this Fusion installation folder instead
rem    install.bat /silent           do not pause at the end
rem
rem  Run it as an administrator if the script reports that it could not write the files.
rem ============================================================================================

setlocal EnableExtensions EnableDelayedExpansion
set "HERE=%~dp0"
set "PAUSE_AT_END=1"
set "FORCED="
set "FOUND_ANY="

:parse
if "%~1"=="" goto after_parse
if /i "%~1"=="/silent" (set "PAUSE_AT_END=0") else (set "FORCED=%~1")
shift
goto parse
:after_parse

set "EDITTIME_SRC=%HERE%edittime\INI++.mfx"
set "RUNTIME_SRC=%HERE%runtime\INI++.mfx"

if not exist "%EDITTIME_SRC%" (
	echo [ERROR] %EDITTIME_SRC% is missing - extract the whole archive first.
	goto end
)
if not exist "%RUNTIME_SRC%" (
	echo [ERROR] %RUNTIME_SRC% is missing - extract the whole archive first.
	goto end
)

if defined FORCED (
	call :install "%FORCED%" "folder given on the command line"
	goto end
)

call :try "SOFTWARE\Clickteam\Fusion Developer 2.5\Settings"          "Fusion 2.5 Developer" "InstallPath"
call :try "SOFTWARE\Clickteam\Fusion 2.5\Settings"                    "Fusion 2.5 Standard"  "InstallPath"
call :try "SOFTWARE\Clickteam\Multimedia Fusion Developer 2\Settings" "MMF2 Developer"       "ProPath"
call :try "SOFTWARE\Clickteam\Multimedia Fusion 2\Settings"           "MMF2 Standard"        "StdPath"

if not defined FOUND_ANY (
	echo.
	echo No Clickteam Fusion 2.5 / MMF2 installation was found in the registry.
	echo Install manually ^(see docs\INSTALL.md^), or give the folder explicitly, e.g.:
	echo     install.bat "C:\Program Files ^(x86^)\Clickteam Fusion 2.5"
)

:end
echo.
echo Ini++ installation finished. Restart Fusion so it picks up the extension.
echo On Android: copy INI++.zip into ^<Fusion^>\Data\Runtime\Android\ ^(see docs\INSTALL.md^).
if "%PAUSE_AT_END%"=="1" pause
endlocal
exit /b 0

rem ---------------------------------------------------------------------------------------------
rem  :try <registry key under HKLM> <friendly name> <value name>
rem ---------------------------------------------------------------------------------------------
:try
for /f "tokens=2,*" %%A in ('reg query "HKLM\%~1" /v "%~3" /reg:32 2^>nul ^| findstr /i "REG_SZ"') do set "ROOT=%%B"
if not defined ROOT exit /b 0
call :trim ROOT
call :install "%ROOT%" "%~2"
exit /b 0

rem ---------------------------------------------------------------------------------------------
rem  :install <fusion folder> <label>
rem ---------------------------------------------------------------------------------------------
:install
set "ROOT=%~1"
call :trim ROOT
set "LABEL=%~2"
if not exist "%ROOT%\" (
	echo [SKIP] %LABEL%: "%ROOT%" does not exist
	exit /b 0
)
set "EDIT_DIR=%ROOT%\Extensions\Unicode"
set "RUN_DIR=%ROOT%\Data\Runtime\Unicode"
if not exist "%EDIT_DIR%\" (
	echo [SKIP] %LABEL%: "%EDIT_DIR%" was not found - not a Unicode Fusion installation?
	exit /b 0
)
echo [OK]   %LABEL%: %ROOT%
call :copy "%EDITTIME_SRC%" "%EDIT_DIR%\INI++.mfx" "edittime" "Extensions\Unicode"
if not exist "%RUN_DIR%\" mkdir "%RUN_DIR%" 2>nul
call :copy "%RUNTIME_SRC%" "%RUN_DIR%\INI++.mfx" "runtime" "Data\Runtime\Unicode"
set "FOUND_ANY=1"
exit /b 0

rem ---------------------------------------------------------------------------------------------
rem  :copy <source> <destination> <kind> <relative folder>
rem ---------------------------------------------------------------------------------------------
:copy
copy /y "%~1" "%~2" >nul 2>nul
if errorlevel 1 (
	echo        [WARN] could not write %~4\INI++.mfx - run this script as administrator
	exit /b 1
)
echo        copied %~3 MFX to %~4\INI++.mfx
exit /b 0

rem ---------------------------------------------------------------------------------------------
rem  :trim <variable name>  -  reg.exe pads its values with trailing spaces
rem ---------------------------------------------------------------------------------------------
:trim
if not defined %~1 exit /b 0
set "VALUE=!%~1!"
:trim_loop
if not "!VALUE:~-1!"==" " goto trim_done
set "VALUE=!VALUE:~0,-1!"
goto trim_loop
:trim_done
set "%~1=!VALUE!"
exit /b 0
