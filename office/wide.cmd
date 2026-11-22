@echo off
set "PY=python"
where pythonw >nul 2>nul
if %errorlevel% equ 0 set "PY=pythonw"
"%PY%" "%~dp0wide.py" %*
