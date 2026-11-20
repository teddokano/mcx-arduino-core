@echo off
rem Upload with probe-rs: Tools > Upload method > probe-rs (boards.txt).
rem The counterpart of upload.bat, which uses LinkServer; same logic as
rem upload_probe-rs.sh.
rem
rem Usage: upload_probe-rs.bat <elf> <probe-rs chip> <port serial number> <port vid> <port pid> <bundled probe-rs's folder>
set "ELF=%~1"
set "CHIP=%~2"
set "PORT_SERIAL=%~3"
set "PORT_VID=%~4"
set "PORT_PID=%~5"
set "BUNDLED=%~6"

rem probe-rs: MCX_PROBE_RS if set, then the one that came with the package
rem ({runtime.tools.probe-rs.path}, unexpanded in braces when it isn't
rem installed), then PATH, then where its own installer puts it
rem (gdb-bridge's findProbeRs() looks in the same places)
set "PROBE_RS="
if defined MCX_PROBE_RS if exist "%MCX_PROBE_RS%" set "PROBE_RS=%MCX_PROBE_RS%"
if not defined PROBE_RS if defined BUNDLED if not "%BUNDLED:~0,1%"=="{" if exist "%BUNDLED%\probe-rs.exe" set "PROBE_RS=%BUNDLED%\probe-rs.exe"
if not defined PROBE_RS for %%i in (probe-rs.exe) do if not "%%~$PATH:i"=="" set "PROBE_RS=%%~$PATH:i"
if not defined PROBE_RS if exist "%USERPROFILE%\.cargo\bin\probe-rs.exe" set "PROBE_RS=%USERPROFILE%\.cargo\bin\probe-rs.exe"
if defined PROBE_RS goto :found
echo ============================================
echo ERROR: probe-rs not found. It comes with the board package:
echo reinstall the package in Boards Manager, install probe-rs
echo from https://probe.rs, or choose Tools ^> Upload method ^> LinkServer.
echo ============================================
exit /b 1

:found
echo Using: %PROBE_RS%

rem The selected port's USB serial number is the MCU-LINK's own, and with
rem the port's VID and PID it makes the VID:PID:SERIAL that probe-rs's
rem --probe takes, to pick the board when more than one is connected. Left
rem out when no port was selected (the placeholder arrives unexpanded), when
rem the port isn't an NXP probe, or when probe-rs doesn't list that serial
rem number.
set "PROBE_ARGS="
if not defined PORT_SERIAL goto :probe_done
if "%PORT_SERIAL:~0,1%"=="{" goto :probe_done
if /i not "%PORT_VID%"=="0x1FC9" goto :probe_done
"%PROBE_RS%" list 2>nul | findstr /c:"%PORT_SERIAL%" >nul
if errorlevel 1 (
    echo The port's serial number %PORT_SERIAL% is not among probe-rs's probes; uploading without --probe
    goto :probe_done
)
set "PID=%PORT_PID:~2%"
set "PROBE_ARGS=--probe 1fc9:%PID%:%PORT_SERIAL%"
echo Probe: 1fc9:%PID%:%PORT_SERIAL%
:probe_done

rem Reset in the same session (--reset), and once more if it fails: see
rem upload_probe-rs.sh
"%PROBE_RS%" download --chip %CHIP% %PROBE_ARGS% --verify --reset --disable-progressbars "%ELF%"
if not errorlevel 1 exit /b 0
echo (once more)
"%PROBE_RS%" download --chip %CHIP% %PROBE_ARGS% --verify --reset --disable-progressbars "%ELF%"
if errorlevel 1 goto :failed
exit /b 0

:failed
if not defined PROBE_ARGS (
    echo ============================================
    echo If more than one board is connected, select the port of the board
    echo to upload to ^(Arduino IDE: Tools ^> Port; arduino-cli: -p^).
    echo ============================================
)
exit /b 1
