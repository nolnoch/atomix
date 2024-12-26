@ECHO OFF

set INDIR="install"
set DATADIR="package\atomix\packages\com.nolnoch.atomix\data"
set METADIR="package\atomix\packages\com.nolnoch.atomix\meta"
set CFGDIR="package\atomix\config"

md %CFGDIR%
md %METADIR%
md %DATADIR%\configs
md %DATADIR%\shaders
md %DATADIR%\res

xcopy /s %INDIR%\* %DATADIR%\
xcopy /s ..\configs %DATADIR%\configs
xcopy /s ..\shaders %DATADIR%\shaders
xcopy /s ..\res %DATADIR%\res

xcopy ..\doc\deploy\win\meta\* %METADIR%\
xcopy ..\doc\deploy\win\config.xml %CFGDIR%\
