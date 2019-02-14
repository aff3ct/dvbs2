@echo on

cd examples

set i=1

:Loop
IF "%~1"=="" goto End

   if    "%i%" ==  "1" goto Example
   if    "%i%" ==  "2" goto Params
   if /i "%i%" GEQ "3" goto Command

:EndLoop
shift
set /A i=i+1
goto Loop

:Example
set example=%~1%
cd %example%
goto EndLoop

:Params
set params=%~1%
goto EndLoop

:Command
set build=%~1%
cd %build%/bin/
my_project.exe %params%
if %ERRORLEVEL% neq 0 exit %ERRORLEVEL%
cd ../..
goto EndLoop

:End
exit /B 0