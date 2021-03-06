Starting with WinDriver 12.70 WinDriver supports PCI & USB driver development for the Windows 10 IoT Core operating system. This technical document explains how to get started with developing a WinDriver-based driver on this OS.
This technical document is for Windows IoT Core only. Please note that Windows 10 IoT Enterprise is a full version of Windows 10, so the regular WinDriver for Windows desktop / server can be used.

Prerequisites
=============
WinDriver must be installed on a development computer running a desktop version of Windows, in order to use DriverWizard.
A network connection allowing an SSH session with the target computer.
IMPORTANT NOTICE:
Connect to your Win10 IoT Core device using an SSH client (such as MobaXterm or PuTTY). DO NOT use PowerShell as some of the WinDriver command line programs and scripts may not work on it.

Installing WinDriver
====================
1. On your Win10 IoT Core create a folder for WinDriver files. Copy the following files to this folder:

a. All files in the WinDriver\redist\WIN10_IOT\<YOUR_PLATFORM> (x86/x64/ARM) directory.
b. The WinDriver\redist\WIN10_IOT\wdreg_iot.cmd script.
c. The INF file for your PCI/USB device that was generated by DriverWizard utility on your development machine.

2. Install WinDriver virtual device using wdreg_iot.cmd script:
wdreg_iot.cmd install_virtual windrvr<WD_VERSION>.inf (for example wdreg_iot.cmd install_virtual windrvr1270.inf)

Uninstalling WinDriver
======================
1. In order to uninstall WinDriver you need to determine the oemXXX.inf file names of WinDriver virtual device and your PCI/USB device. To do so run the following command:
wdreg_iot.cmd enum

2. Uninstall your PCI/USB device by running:
wdreg_iot.cmd uninstall
Note that VID/PID should always be 4 digits hexadecimal numbers. For example:
wdreg_iot.cmd uninstall 09D9 0010 oem1.inf

3. Uninstall WinDriver virtual device by running:
wdreg_iot.cmd uninstall_virtual
For example:
wdreg_iot.cmd uninstall_virtual oem0.inf

NOTICE: If uninstall fails, you can try using wdreg_iot.cmd uninstall_force which forces uninstallation of the driver and stops all processes related to it.

Creating an INF for your device driver
======================================
If you can connect your PCI/USB device to your development computer you can create an INF as described in the DriverWizard Walkthrough in the WinDriver user manual.
If you can???t actually connect your PCI device to your development computer then you can create a project for a ???Virtual Device??? and then transfer the project files to your Win10 IoT Core device.
Create an INF for your device with DriverWizard:
1. Go to File->New Device Driver Project
2. Select PCI Virtual Device
3. Click Generate INF
4. Enter your device???s VID/PID, Manufacturer and Device Name.

Installing your WinDriver-generated driver
==========================================
1. Copy the INF you have created to your Win10 IoT Core device, and put it in the directory that holds wdreg_iot.cmd.
2. Install WinDriver for your PCI/USB device using wdreg_iot.cmd script:
wdreg_iot.cmd install

Running a sample application
1. Copy pci_diag.exe from WinDriver\samples\pci_diag\ or usb_diag.exe from WinDriver\samples\usb_diag\. Make sure you are using the suitable version for your platform (x86/x64/ARM).
2. Copy wdapi<WD_VERSION>.dll (for example wdapi1270.dll) from WinDriver\redist. Make sure you are using the suitable version for your platform (x86/x64/ARM).
2. Run the program from your WinDriver directory.

NOTICE: pci_diag will only show devices that you have already installed a WinDriver generated INF for.

Running your generated code
===========================
1. Generate code as described in the DriverWizard Walkthrough, and modify it to suit your need.

NOTICE: Since Win10 IoT Core currently does not support WinForms applications, the DriverWizard generated code in C#/Visual Basic (which uses these libraries) will not run on this environment. Therefore, you must generate code in C.

2. Compile your code on the development computer. Make sure you compile it to suit your target platform.
3. Copy your compiled binary to the device and run it using SSH.

Collecting WinDriver logs using Debug Monitor utility:
======================================================
1. Copy WinDriver\samples\wddebug\wddebug.exe to your Win10 IoT Core device
2. Run wddebug dump
3. Run your application
4. Stop wddebug utility using CTRL+C.