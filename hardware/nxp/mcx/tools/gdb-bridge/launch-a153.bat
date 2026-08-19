@echo off
rem Hands the Windows gdb-bridge binary FRDM-MCXA153's LinkServer device
rem string. See gdb-bridge/src/main.go for what this does and why.
set DIR=%~dp0
"%DIR%gdb-bridge-windows-amd64.exe" "MCXA153:FRDM-MCXA153" %*
