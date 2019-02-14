@echo on

set "VSCMD_START_DIR=%CD%"
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"

rem Compile the AFF3CT library
cd lib\aff3ct
mkdir %BUILD%
cd %BUILD%
cmake .. -G"Visual Studio 15 2017 Win64" -DCMAKE_CXX_FLAGS="%CFLAGS% /MP%THREADS%" -DAFF3CT_COMPILE_EXE="OFF" -DAFF3CT_COMPILE_STATIC_LIB="ON" -DCMAKE_INSTALL_PREFIX="install"
if %ERRORLEVEL% neq 0 exit %ERRORLEVEL%
devenv /build Release aff3ct.sln
if %ERRORLEVEL% neq 0 exit %ERRORLEVEL%
devenv /build Release aff3ct.sln /project INSTALL > nul
if %ERRORLEVEL% neq 0 exit %ERRORLEVEL%
cd ..

rem Compile all the projects using AFF3CT
cd ..\..\examples
for %%a in (%EXAMPLES%) do (
	cd %%a
	call :compile_my_project
	if %ERRORLEVEL% neq 0 exit %ERRORLEVEL%
	cd ..
)

exit /B %ERRORLEVEL%

:compile_my_project
mkdir cmake-config
xcopy ..\..\lib\aff3ct\%BUILD%\lib\cmake\aff3ct-%AFF3CT_GIT_VERSION%\* cmake-config\ /s /e
mkdir %BUILD%
cd %BUILD%
cmake .. -G"Visual Studio 15 2017 Win64" -DCMAKE_CXX_FLAGS="%CFLAGS% /MP%THREADS%"
if %ERRORLEVEL% neq 0 exit %ERRORLEVEL%
devenv /build Release my_project.sln
if %ERRORLEVEL% neq 0 exit %ERRORLEVEL%
copy bin\Release\my_project.exe bin\
rd /s /q "bin\Release\"
cd ..
exit /B 0
