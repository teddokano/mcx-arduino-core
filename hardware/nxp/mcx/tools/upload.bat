@echo off
set ELF=%1
set LINKSERVER_TARGET=%2
set PORT_SERIAL=%~3
set PORT_VID=%~4

rem The selected port's USB serial number, which for an NXP probe (MCU-Link,
rem VID 0x1FC9) is the probe serial LinkServer wants. Without it, LinkServer
rem refuses to flash while more than one board is plugged in. Left out for
rem any other port, and when no port was selected (the placeholder then
rem arrives unexpanded, still in braces), so one board keeps working as
rem before. Same logic as upload.sh.
set PROBE_ARGS=
if not defined PORT_SERIAL goto :probe_done
if "%PORT_SERIAL:~0,1%"=="{" goto :probe_done
if /i not "%PORT_VID%"=="0x1FC9" goto :probe_done
set PROBE_ARGS=--probe %PORT_SERIAL%
:probe_done

for /f "delims=" %%i in ('dir /b /ad "C:\NXP\LinkServer*" 2^>nul ^| sort /r') do (
    set LINKSERVER=C:\NXP\%%i\LinkServer.exe
    goto :found
)

echo ============================================
echo ERROR: LinkServer not found.
echo Please install LinkServer from:
echo https://www.nxp.com/linkserver
echo ============================================
exit /b 1

:found
echo Using: %LINKSERVER%
if defined PROBE_ARGS echo Probe: %PORT_SERIAL%
"%LINKSERVER%" flash %PROBE_ARGS% %LINKSERVER_TARGET% load "%ELF%"
set STATUS=%ERRORLEVEL%

rem LinkServer's own message asks for --probe, which an IDE user can't pass.
if %STATUS% neq 0 if not defined PROBE_ARGS (
    echo ============================================
    echo If more than one board is connected, select the port of the board
    echo to upload to ^(Arduino IDE: Tools ^> Port; arduino-cli: -p^).
    echo ============================================
)
exit /b %STATUS%
