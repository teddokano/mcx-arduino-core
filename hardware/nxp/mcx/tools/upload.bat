@echo off
set ELF=%1

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
"%LINKSERVER%" flash MCXA153:FRDM-MCXA153 load "%ELF%"
