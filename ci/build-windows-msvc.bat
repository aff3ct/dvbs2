@echo on

set "VSCMD_START_DIR=%CD%"
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"

mkdir %BUILD%
cd %BUILD%
cmake .. -G"Visual Studio 15 2017 Win64" -DCMAKE_CXX_FLAGS="%CFLAGS% /MP%THREADS%"
if %ERRORLEVEL% neq 0 exit %ERRORLEVEL%
devenv /build Release dvbs2_optique.sln
if %ERRORLEVEL% neq 0 exit %ERRORLEVEL%
exit /B 0
