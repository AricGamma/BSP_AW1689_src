echo Start to build drivers...

call "%WDKCONTENTROOT%\Tools\bin\i386\installoemcerts.cmd"

if exist "%WorkRoot%\src\Build\%DriverBuildType%" (
	rd /s /q "%WorkRoot%\src\Build\%DriverBuildType%"
)

call "msbuild" "%WorkRoot%\src\Build\%Chipset%.sln" /t:rebuild /p:platform=%CpuPlatform% /p:configuration="%DriverBuildType%" /m:8 /nologo

if NOT %errorlevel% == 0 (
	echo there are some errors when build drivers .
	pause
) else (
	echo Build successfully.
)

cd "%WorkRoot%\src\Build\%DriverBuildType%"


for /f "delims==" %%i in ('dir /S /B *.cab') do (
	copy "%%i" "%SocPrebuiltCabPath%"
)



