@echo off
py -3 "%~dp0..\..\scripts\bootstrap_tools.py" gradle %*
exit /b %errorlevel%
