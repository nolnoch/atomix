@ECHO OFF

set CFG=%1
set QTDEPLOY="C:\Qt\6.8.1\msvc2022_64\bin\windeployqt.exe"
set QTBINARY="C:\Qt\Tools\QtInstallerFramework\4.8\bin\binarycreator.exe"
set QTBINBASE="C:\Qt\Tools\QtInstallerFramework\4.8\bin\installerbase.exe"
set INDIR="install"
set DATADIR="deploy\packages\com.nolnoch.atomix\data"
set METADIR="deploy\packages\com.nolnoch.atomix\meta"
set CFGDIR="deploy\config"

@REM echo "%QTCMD% --dir=%INDIR% src\%CFG%\atomix.exe"
%QTDEPLOY% --dir=%INDIR% src\%CFG%\atomix.exe

md %CFGDIR%
md %METADIR%
md %DATADIR%\configs
md %DATADIR%\shaders
md %DATADIR%\res

xcopy /s /e %INDIR%\* %DATADIR%\
xcopy /s /e ..\configs %DATADIR%\configs
xcopy /s /e ..\shaders %DATADIR%\shaders
xcopy /s /e ..\res %DATADIR%\res

xcopy ..\doc\deploy\win\meta\* %METADIR%\
xcopy ..\doc\deploy\win\config.xml %CFGDIR%\

echo "Done installing."
echo "Begin packaging."
cd deploy

%QTBINARY% --offline-only -t %QTBINBASE% -c config\config.xml -p packages atomix-installer-win64.exe

echo "Done packaging."
