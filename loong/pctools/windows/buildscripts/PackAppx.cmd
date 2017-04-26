echo Start to pack appx......

call %IotAddOnPath%\Tools\launch.cmd %CpuPlatform%

cd %AppxSourcePath%

del /f /q %PKGBLD_DIR%\*.cab

for /f "delims==" %%i in ('dir /ad /b') do (

    cd %AppxSourcePath%\%%i

    for /f "delims==" %%j in ('dir /s /b AppPackages') do (
    
        cd %%j\%%i*

        echo Create App.%%i package....
            
        call %IotAddOnPath%\Tools\newappxpkg.cmd *.appx Appx.%%i

        call %IotAddOnPath%\Tools\buildpkg Appx.%%i

    )
)

call %IotAddOnPath%\Tools\buildpkg Custom.Cmd
call %IotAddOnPath%\Tools\buildpkg Provisioning.Auto
call %IotAddOnPath%\Tools\buildpkg Registry.Version
call %IotAddOnPath%\Tools\buildpkg OemTools.InstallTools
call %IotAddOnPath%\Tools\createpkg Registry.ConfigSettings

cd %PKGBLD_DIR%


echo Copy appx package to package directory

for /f "delims==" %%k in ('dir /s /b *.cab') do (
    copy "%%k" "%SocPrebuiltCabPath%"
)

