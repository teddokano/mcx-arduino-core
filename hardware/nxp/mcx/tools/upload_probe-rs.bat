@echo off
rem Upload with probe-rs: Tools > Upload method > probe-rs (boards.txt).
rem The counterpart of upload.bat, which uses LinkServer; same logic as
rem upload_probe-rs.sh.
rem
rem Usage: upload_probe-rs.bat <elf> <probe-rs chip> <port serial number> <port vid> <port pid> <bundled probe-rs's folder> [verify|noverify]
set "ELF=%~1"
set "CHIP=%~2"
set "PORT_SERIAL=%~3"
set "PORT_VID=%~4"
set "PORT_PID=%~5"
set "BUNDLED=%~6"

rem Read back and check what was written unless Arduino IDE's "Verify code
rem after upload" is off (noverify): see upload_probe-rs.sh
set "VERIFY_ARG=--verify"
if /i "%~7"=="noverify" set "VERIFY_ARG="

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

rem Reset in the same session (--reset), and tried up to three times, a
rem second apart, with what a failed try says kept back and shown only if
rem every try fails: see upload_probe-rs.sh.
rem All it says goes to stdout: it writes it to stderr, which Arduino IDE
rem shows as an error. NO_COLOR takes out its color codes, though not its
rem bold ones; a failure is said on stderr below. stdin is closed (<nul),
rem so that probe-rs doesn't wait to be told which probe to use
set NO_COLOR=1
rem The SWD clock: 4MHz unless PROBE_RS_SPEED (kHz) says otherwise, see
rem upload_probe-rs.sh
if not defined PROBE_RS_SPEED set PROBE_RS_SPEED=4000
set "TRY_LOG=%TEMP%\mcx_probe-rs_%RANDOM%.txt"
set "FAILED_LOG=%TRY_LOG%.failed"
type nul > "%FAILED_LOG%"
set ATTEMPT=1
:try
"%PROBE_RS%" download --chip %CHIP% %PROBE_ARGS% %VERIFY_ARG% --reset --disable-progressbars "%ELF%" < nul > "%TRY_LOG%" 2>&1
if not errorlevel 1 goto :written
>> "%FAILED_LOG%" echo --- try %ATTEMPT% of 3 ---
type "%TRY_LOG%" >> "%FAILED_LOG%"
if %ATTEMPT% GEQ 3 goto :failed
set /a ATTEMPT+=1
rem a second's wait; timeout fails without a console, ping doesn't
ping -n 2 127.0.0.1 >nul
goto :try

:written
if %ATTEMPT% EQU 2 echo Written on try 2 of 3 ^(the first try failed; what it said is left out^)
if %ATTEMPT% EQU 3 echo Written on try 3 of 3 ^(the tries before it failed; what they said is left out^)
type "%TRY_LOG%"
del "%TRY_LOG%" "%FAILED_LOG%" 2>nul
exit /b 0

:failed
type "%FAILED_LOG%"
del "%TRY_LOG%" "%FAILED_LOG%" 2>nul
echo ERROR: probe-rs could not write the board in 3 tries ^(above: what it said each time^) 1>&2
if not defined PROBE_ARGS (
    echo ============================================
    echo If more than one board is connected, select the port of the board
    echo to upload to ^(Arduino IDE: Tools ^> Port; arduino-cli: -p^).
    echo ============================================
)
exit /b 1
