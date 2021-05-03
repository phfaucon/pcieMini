@echo off

set argc=0
for %%x in (%*) do set /A argc+=1

if not %argc% == 1 if not %argc% == 2 if not %argc% == 3 if not %argc% == 4 (
        echo invalid number of arguments '%argc%'
        goto USAGE
)

set op=%1
set dev=%2

if %op% == install_virtual (
        goto INSTALL_VIRTUAL
) else if %op% == install (
        goto INSTALL
) else if %op% == uninstall_virtual (
        goto UNINSTALL_VIRTUAL
) else if %op% == uninstall (
        goto UNINSTALL
) else if %op% == uninstall_force (
        goto UNINSTALL_FORCE
) else if %op% == enum (
        goto ENUM
) else if %op% == find_usb (
        goto FIND_USB
) else if %op% == find_pci (
        goto FIND_PCI
) else if %op% == find_virtual (
        goto FIND_VIRTUAL
) else if %op% == help (
        goto USAGE
) else (
        echo Unknown option '%op%'
        goto USAGE
)

:INSTALL_VIRTUAL
        echo Installing WinDriver virtual device (%dev%)...
        devcon install %dev% *WINDRVR1440
        goto EXIT

:INSTALL
        echo Installing %dev%...
        devcon dp_add %dev%
        goto EXIT

:UNINSTALL_VIRTUAL
        echo Uninstalling WinDriver virtual device...
        set oem_inf=%2
        devcon remove *WINDRVR1440
        devcon dp_delete %oem_inf%
        goto EXIT

:UNINSTALL
        set vid=%2
        set pid=%3
        set oem_inf=%4
        echo Uninstalling %vid%/%pid% device...
        devcon remove *_%vid%*_%pid%
        devcon dp_delete %oem_inf%
        goto EXIT

:UNINSTALL_FORCE
        echo Forcing uninstall of the driver [%2]...
        pnputil /delete-driver %2 /uninstall /force
        goto EXIT

:ENUM
        pnputil /enum-drivers
        goto EXIT

:FIND_USB
        devcon find USB*
        goto EXIT

:FIND_PCI
        devcon find PCI*
        goto EXIT

:FIND_VIRTUAL
        devcon find *WINDRVR1440
        goto EXIT

:USAGE
        echo Usage:
        echo     wdreg_iot.cmd install_virtual ^<INF file name^>                  Install WinDriver virtual device
        echo     wdreg_iot.cmd install ^<INF file name^>                          Install USB/PCI device
        echo     wdreg_iot.cmd uninstall_virtual ^<OEM INF file name^>            Uninstall virtual device.
        echo     wdreg_iot.cmd uninstall ^<VID^> ^<PID^> ^<OEM INF file name^>        Uninstall USB/PCI device.
        echo     wdreg_iot.cmd uninstall_force ^<OEM INF file name^>              Force Uninstall for any device.
        echo     wdreg_iot.cmd enum                                             List OEM INF files for installed devices
        echo     wdreg_iot.cmd find_virtual                                     Find HW IDs of installed WinDriver virtual devices
        echo     wdreg_iot.cmd find_pci                                         Find HW IDs of attached PCI devices
        echo     wdreg_iot.cmd find_usb                                         Find HW IDs of attached USB devices
        echo     wdreg_iot.cmd help                                             Display this message
        echo Examples:
        echo     wdreg_iot.cmd install_virtual windrvr1440.inf
        echo     wdreg_iot.cmd install my_dev.inf
        echo     wdreg_iot.cmd uninstall_virtual oem0.inf
        echo     wdreg_iot.cmd uninstall 09D9 0010 oem1.inf
        goto EXIT

:EXIT

