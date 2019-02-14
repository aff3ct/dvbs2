@echo on

rem Compile the AFF3CT library
cd lib\aff3ct
mkdir %BUILD%
cd %BUILD%
cmake .. -G"MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++.exe -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="%CFLAGS%" -DAFF3CT_COMPILE_EXE="OFF" -DAFF3CT_COMPILE_STATIC_LIB="ON" -DCMAKE_INSTALL_PREFIX="install"
if %ERRORLEVEL% neq 0 exit %ERRORLEVEL%
mingw32-make -j %THREADS%
if %ERRORLEVEL% neq 0 exit %ERRORLEVEL%
mingw32-make install > nul
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
cmake .. -G"MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++.exe -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="%CFLAGS%"
if %ERRORLEVEL% neq 0 exit %ERRORLEVEL%
mingw32-make -j %THREADS%
if %ERRORLEVEL% neq 0 exit %ERRORLEVEL%
cd ..
exit /B 0
