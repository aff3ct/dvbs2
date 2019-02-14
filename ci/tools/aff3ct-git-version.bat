@echo off

cd lib/aff3ct/

for /F "tokens=* USEBACKQ" %%F in (`git describe`) do (
	set "AFF3CT_GIT_VERSION=%%F"
)
set AFF3CT_GIT_VERSION=%AFF3CT_GIT_VERSION:~1%

cd ../../

:End