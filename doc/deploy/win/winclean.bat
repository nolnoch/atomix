@ECHO OFF

rd /s /q package\atomix

del /s /f /q install\*.*
for /f %%f in ('dir /ad /b install') do rd /s /q install\%%f

del deploy.bat
