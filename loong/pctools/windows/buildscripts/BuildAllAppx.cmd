echo Start to build Appx....

call "%WDKCONTENTROOT%\Tools\bin\i386\installoemcerts.cmd"


cd "%AppxSourcePath%"

for /f "delims==" %%i in ('dir /s /b *.sln') do (

    call "msbuild" %%i /t:rebuild /p:platform=%CpuPlatform% /p:configuration=%DriverBuildType% /m:8 /nologo

    if NOT %errorlevel% == 0 (

        echo there are some errors when build drivers .
        pause

    ) else (
        echo Build "%%i" successfully
    )
)
