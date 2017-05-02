# Windows 10 IoT for Allwinner

---

## Introduction

本项目主要介绍如何在Allwinner平台下，开发、定制、打包Windows 10 IoT Core镜像。  
项目主要包含：

1. 内核驱动源代码
2. UWP app 示例代码
3. 用于BOOT的UEFI二进制文件
4. 生成ffu镜像的脚本和FeatureManifest配置文件

## 主要内容:

- [1. 目录结构](#1)
- [2. 如何构建](#2)
- [3. 如何部署](3)
- [4. 更新内容](#4)
- [5. 已知问题](#5)

<h2 id="1">1. 目录结构</h2>

        Root
          |-- .\loong                                         包含打包脚本和配置文件以及生成cab包和ffu镜像的目录
          *    |-- .\chips                                    包含特定CPU平台的配置文件和启动配置文件(BCD)
          *    *    |-- .\aw1689                              Allwinner A64平台目录
          *    *    *    |-- .\bin    
          *    *    *    *    |-- .\EFI                       启动配置文件(BCD)
          *    *    *    *    |-- *.bin                       过期的二进制文件
          *    *    *    |-- .\boot-resource                  boot资源文件(已过期)
          *    *    *    |-- .\devices                        不同设备的feature manifest和ACPI表
          *    *    *    *    |-- .\BPI-M64                   BananaPi M64开发板的配置文件
          *    *    *    *    *    |-- .\acpi                 包含acpi相关配置和生成目录
          *    *    *    *    *    *    |-- .\ACPI            ACPI生成目录
          *    *    *    *    *    *    |-- .\inc             包含目录
          *    *    *    *    *    *    |-- .\src             源文件目录
          *    *    *    *    *    |-- .\logo                 boot阶段显示logo
          *    *    *    *    *    |-- DeviceFM.xml           设备feature manifest
          *    *    *    *    *    |-- DeviceInfo.pkg.xml     设备信息打包文件
          *    *    *    *    *    |-- DeviceLayout.xml       设备分区信息
          *    *    *    *    *    |-- DevicePlatform.xml     平台信息
          *    *    *    *    *    |-- OemInput_RetailOS.xml  用于打包RetailOS的FM文件
          *    *    *    *    *    |-- OemInput_TestOS.xml    用于打包TestOS的FM文件
          *    *    *    *    |-- .\default                   aw1689平台通用配置文件
          *    *    *    *    |-- .\dingdong                  内部测试开发板
          *    *    *    *    |-- .\perf2_v1_0                内部测试开发板
          *    *    *    *    |-- .\pine64                    Pine64开发板的配置文件
          *    *    *    *    |-- .\r18                       内部测试板
          *    *    *    |-- .\tools                          一些工具(已过期)
          *    |-- .\common                                   过期的文件
          *    |-- .\ffu                                      ffu生成目录
          *    |-- .\pctools                                  包含编译、打包、debug、烧写UEFI等脚本工具
          *    *    |-- .\linux                               linux下的一些脚本工具(已过期)
          *    *    |-- .\windows                             windows下使用的脚本和工具
          *    *    *    |-- .\acpi                           acpi提取工具
          *    *    *    *    |-- .\acpi_extract.exe          acpi提取工具
          *    *    *    |-- .\buildscripts                   编译驱动和app、打包ffu脚本目录(由Build.cmd调用)
          *    *    *    *    |-- .\iot-adk-addonkit          引用自(https://github.com/ms-iot/iot-adk-addonkit)
          *    *    *    *    |-- BuildAllAppx.cmd            编译位于.\src\app目录下的所有app
          *    *    *    *    |-- BuildAllComponent.cmd       编译所有组件
          *    *    *    *    |-- BuildBootloader.cmd         编译bootloader(已过期)
          *    *    *    *    |-- BuildWindowsDrivers.cmd     编译驱动并生成cab包
          *    *    *    *    |-- PackAppx.cmd                将编译好的app打包成cab包
          *    *    *    *    |-- PackBootloader.cmd          打包bootloader
          *    *    *    *    |-- PackBootresource.cmd        将启动资源文件打包成cab包
          *    *    *    *    |-- PackDeviceConfigs.cmd       将设备配置文件打包成cab包
          *    *    *    *    |-- PackFfu.cmd                 生成ffu镜像
          *    *    *    |-- .\UefiUpgrade                    安全地更新UEFI的脚本工具
          *    *    *    |-- .\VirEth                         虚拟以太网与USB桥接工具(用于debug)
          *    *    *    *    |-- VirEth_TH2.exe              For Build 10586 1511
          *    *    *    *    |-- VirEth_RS1.exe              For Build 14393 1607
          *    |-- .\prebuilt                                 包含生成的cab包和UEFI二进制文件，用于构建ffu
          *    *    |-- .\aw1689                              Allwinner A64平台目录
          *    *    *    |-- .\CabPackages                    平台通用cab包生成目录
          *    *    *    |-- .\devices                        同CPU平台不同设备的cab包生成目录
          |-- .\src                                           驱动和示例app的源代码
          *    |-- .\app                                      UWP app示例源代码
          *    |-- .\build                                    包含所有driver工程的统一解决方案(.sln)以及驱动生成目录
          *    |-- .\drivers                                  所有驱动工程的源代码
          |-- Build.cmd                                       打包流程入口脚本
          |-- SetBuildEnv.cmd                                 设置必要的环境变量(由Build.cmd调用)


<h2 id="2">2. 如何构建</h2>

### 1. 构建前的准备

(1). 将您的操作系统升级至1607版本(Build 14393)，并确保系统盘可用空间在50GB以上

(2). 安装[Visual Studio 2015](https://www.visualstudio.com)

> PS: 安装时请勾选Universal Windows App Development Tools及其子项；请不要修改安装路径。

(3). 安装 [Windows 10 driver kits(WDK)](https://developer.microsoft.com/zh-cn/windows/hardware/windows-driver-kit)

(4). 安装 [Windows ADK for Windows 10](https://developer.microsoft.com/en-us/windows/hardware/windows-assessment-deployment-kit)

(5). 安装 [Windows 10 IoT Kits iso](https://msdn.microsoft.com/subscriptions/json/GetDownloadRequest?brand=MSDN&locale=zh-cn&fileId=70177&activexDisabled=false&akamaiDL=false)

> PS: 需要msdn订阅账户才能下载。

### 2. 编译驱动文件

以管理员身份打开Visual Studio 2015，点击菜单中的"文件 -> 打开 -> 项目/解决方案"，选择`.\src\build\aw1689.sln`解决方案文件。打开解决方案后，在菜单中选择"生成 -> 生成解决方案"，等待生成成功后，可以在`.\src\build\ARM`目录下，对应Debug/Release目录下找到所有生成成功的驱动文件和驱动cab包。  

学习如何开发Windows通用驱动程序，请访问 [这里](https://msdn.microsoft.com/windows/hardware/drivers/develop/getting-started-with-universal-drivers)或者查看github上的 [样例代码](https://github.com/Leeway213/Windows-driver-samples)。

### 3. 编译Windows通用应用(UWP)

打开Visual Studio 2015，点击菜单中的"文件 -> 打开 -> 项目/解决方案"，选择`.\src\app\IoTDefaultAppIoTCoreDefaultApp.sln`解决方案文件。打开工程后，在菜单中选择"生成 -> 生成解决方案"，等待生成成功后，可以在`.\src\app\IoTDefaultApp\IoTCoreDefaultApp\bin\ARM\[Debug/Release]\`目录下，找到生成的应用的二进制文件。  

学习如何在Windows 10 IoT Core上开发Windows通用应用程序(UWP), 请访问 [这里](https://msdn.microsoft.com/windows/uwp/get-started/universal-application-platform-guide#tooling)或查看github上的 [样例代码](https://github.com/Leeway213/samples)。

### 4. 生成操作系统ffu镜像

学习如何构建自己的IoT Core ffu，可以访问 [这里](https://msdn.microsoft.com/zh-cn/windows/hardware/commercialize/manufacture/iot/index)。  

为简化生成ffu操作，我们构建了一系列脚本，以帮助您更简单的生成ffu。只需要运行项目根目录下的Build.cmd脚本，输入数字选择对应的选项即可执行相应的生成操作。  

> 例如：输入"1"，即编译生成所有的驱动和应用程序，并打包ffu。  

事实上，Build.cmd首先调用SetBuildEnv.cmd脚本设置了一系列环境变量，通过输入数字选项，调用`.\loong\pctools\windows\buildscripts`目录下对应的cmd脚本。  

**<font color="red">需要注意的一点是：在执行打包ffu过程前，请再三确保拔出所有连接在PC上的大容量存储设备，如U盘、SD卡等！否则，打包程序会彻底销毁您的数据！</font>**

ffu生成成功后，可以在`.\loong\ffu`目录下找到对应设备的.ffu文件。  

> PS: 在SetBuildEnv.cmd脚本中，默认设置的生成设备为pine64开发板，如果想要生成其他已支持的开发板，请打开SetBuildEnv.cmd脚本，修改`set DeviceName=pine64`这一行，设置成你想要生成的设备名称。  
目前已支持的设备有：pine64开发板、BananaPi M64开发板以及Allwinner内部测试的一些开发板。

<h2 id="3">3. 如何部署</h2>

请参考文档 [How to flash ffu](https://github.com/Leeway213/Win10-IoT-for-A64-Release-Notes/blob/master/doc/How%20to%20flash%20ffu.md)。


<h2 id="4">4. 更新内容</h2>

**[2017-04-26]:**  

          第一次上传


<h2 id="5">5. 已知问题</h2>

1. 无法软件重启、关闭操作系统，执行关闭或重启操作时，操作系统会蓝屏，错误代码：0x0000000a

2. Realtek RTK8723 wifi信号和传输速度较差

3. BPI-M64以太网驱动，无法枚举到PHY

如果您有其他问题或建议，欢迎与我们联系： [zhangliwei@allwinnertech.com](mailto:zhangliwei@allwinnertech.com)
