@echo off
set ELF=%1
set LINKSERVER_TARGET=%2
set "BOARD_DEVICE=%~2"
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

rem Every C:\NXP\LinkServer_<version> goes through :consider, which keeps
rem the highest version, compared as numbers (sorting the names would put
rem 26.9 above 26.12), and passes over a version with the FRDM-MCXA153
rem flash-size bug (see there), remembering the highest such.
set "LINKSERVER="
set "BEST_KEY=-1"
set "BUGGY="
set "BUGGY_KEY=-1"
set "BUGGY_VERSION="
set "SKIPPED="
for /f "delims=" %%i in ('dir /b /ad "C:\NXP\LinkServer*" 2^>nul') do (
    if exist "C:\NXP\%%i\LinkServer.exe" call :consider "C:\NXP\%%i\LinkServer.exe" "%%i"
)
if defined LINKSERVER goto :found

rem Only versions with the bug: use one anyway, since other boards are
rem unaffected and an FRDM-MCXA153 sketch that fits in 32KB still loads,
rem and explain if an FRDM-MCXA153 upload fails.
if defined BUGGY goto :use_buggy

echo ============================================
echo ERROR: LinkServer not found.
echo Please install LinkServer from:
echo https://www.nxp.com/linkserver
echo ============================================
exit /b 1

:use_buggy
set "LINKSERVER=%BUGGY%"
set "SKIPPED="
goto :using

:found
set "BUGGY_VERSION="
:using
echo Using: %LINKSERVER%
if not defined SKIPPED goto :skipped_done
echo ^(passed over LinkServer%SKIPPED%, which reads the FRDM-MCXA153's flash as 32KB^)
:skipped_done

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

if %STATUS% equ 0 exit /b 0

if not defined BUGGY_VERSION goto :bug_message_done
if not "%BOARD_DEVICE:~0,8%"=="MCXA153:" goto :bug_message_done
echo ============================================
echo If the error above is "Attempt to load into missing flash area":
echo LinkServer %BUGGY_VERSION% reads the FRDM-MCXA153's flash as 32KB, so a
echo sketch larger than that fails to upload. Install LinkServer 26.6.137
echo alongside it; uploads then use that one automatically. Download links:
echo https://github.com/teddokano/mcx-arduino-core#nxp-linkserver-required-for-uploading-and-debugging
echo ============================================
:bug_message_done

rem LinkServer's own message asks for --probe, which an IDE user can't pass.
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

rem Takes one LinkServer.exe path (%1) and its directory name (%2, which
rem LinkServer's installer makes LinkServer_<version>). Makes it LINKSERVER
rem if its version is the highest so far, unless it is a version whose
rem flash driver takes the FRDM-MCXA153's 128KB of flash for 32KB, so a
rem larger sketch fails to load ("Attempt to load into missing flash
rem area"): those go to BUGGY instead. That is for every board, not just
rem the FRDM-MCXA153: LinkServer leaves its redlinkserv running after a
rem flash, and 26.9 fails ("Redlink interface error 240") when the one it
rem finds was started by 26.6, so all boards have to use the same version.
rem Keep the version list in step with upload.sh and gdb-bridge's
rem flashSizeBug.
:consider
set "LS_VERSION="
set "LS_KEY=0"
for /f "tokens=2-4 delims=_." %%a in ("%~2") do (
    set "LS_VERSION=%%a.%%b.%%c"
    set /a LS_KEY=%%a*1000000+%%b*1000+%%c 2>nul
)
if not "%LS_VERSION:~0,5%"=="26.9." goto :consider_ok
set "SKIPPED=%SKIPPED% %LS_VERSION%"
if %LS_KEY% leq %BUGGY_KEY% exit /b 0
set "BUGGY=%~1"
set "BUGGY_KEY=%LS_KEY%"
set "BUGGY_VERSION=%LS_VERSION%"
exit /b 0
:consider_ok
if %LS_KEY% leq %BEST_KEY% exit /b 0
set "LINKSERVER=%~1"
set "BEST_KEY=%LS_KEY%"
exit /b 0
