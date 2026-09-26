@echo off
set ELF=%1
set LINKSERVER_TARGET=%2
rem Quoted, so a serial number with "&" in it (as a Windows device instance
rem ID can have) stays text instead of splitting the line into commands.
set "PORT_SERIAL=%~3"
set "PORT_VID=%~4"

rem The selected port's USB serial number, which for an NXP probe (MCU-Link,
rem VID 0x1FC9) is the probe serial LinkServer wants. Without it, LinkServer
rem refuses to flash while more than one board is plugged in. Left out for
rem any other port, and when no port was selected (the placeholder then
rem arrives unexpanded, still in braces), so one board keeps working as
rem before. Same logic as upload.sh.
set "PROBE_SERIAL="
if not defined PORT_SERIAL goto :probe_done
if "%PORT_SERIAL:~0,1%"=="{" goto :probe_done
if /i not "%PORT_VID%"=="0x1FC9" goto :probe_done
set "PROBE_SERIAL=%PORT_SERIAL%"
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

rem Only pass --probe when LinkServer lists that serial number, since it
rem refuses to flash at all for one it doesn't know ("No probes matched").
rem A port that reports its serial number in some other form then uploads
rem as before --probe was passed: fine with one board, refused with several.
rem A probe busy with a debug session is still listed, so this doesn't send
rem the upload to another board.
set "PROBE_ARGS="
set "UNLISTED="
if not defined PROBE_SERIAL goto :listed_done
"%LINKSERVER%" probes 2>&1 | findstr /i /l /c:"%PROBE_SERIAL%" >nul
if not errorlevel 1 goto :serial_listed
echo The port's serial number "%PROBE_SERIAL%" is not among LinkServer's probes; uploading without --probe
set "UNLISTED=1"
goto :listed_done
:serial_listed
set "PROBE_ARGS=--probe %PROBE_SERIAL%"
echo Probe: %PROBE_SERIAL%
:listed_done

"%LINKSERVER%" flash %PROBE_ARGS% %LINKSERVER_TARGET% load "%ELF%"
set STATUS=%ERRORLEVEL%

rem LinkServer's own message asks for --probe, which an IDE user can't pass.
if %STATUS% equ 0 exit /b 0
if defined PROBE_ARGS exit /b %STATUS%
echo ============================================
if defined UNLISTED goto :unlisted_message
echo If more than one board is connected, select the port of the board
echo to upload to ^(Arduino IDE: Tools ^> Port; arduino-cli: -p^).
goto :message_done
:unlisted_message
echo The selected port could not be matched to a debug probe, so with
echo more than one board connected, leave only the one to upload to.
:message_done
echo ============================================
exit /b %STATUS%
