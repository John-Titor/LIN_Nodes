Firmware and PCB designs for various LIN network nodes intended for retrofitting older vehicles.

```
boards/
    Eagle schematic and PCB files.

common/
    Protocol definitions and generator.

Documentation/
    Datasheets for various parts used in the design.

lintool
    Host-side configuration tool. Talks to the usb_link hardware.

nodes/
    All of the nodes in this part of the tree are based on the Atmel ATTiny167.

    bl_core/
        LIN bootloader core, consumed by bl_installer.

    bl_installer/
        Bootloader installer; flashed onto a fresh board this will install the
        bootloader.

        The installed bootloader comes up on NAD 32 and can be flashed with lintool
        by explicitly supplying the node address and firmware file.

        lintool update -n 32 <path-to-fw-file>

        Only one node can be in this 'recovery' mode on the network when doing this.

    lib/
        Shared code.

    master/
        Network master node. Supports 7 switch-to-battery and 14 switch-to-ground
        inputs. 4 of the switch-to-ground inputs can also be 0-5V voltage inputs,
        with optional 2 or 16mA current sources for measuring resistive sensors.

    power_v3/
        5-output power switching node using Freescale smart high-side switches.
        Board limited to 5.5A, but code should work with the 10A device as well.
        Outputs support PWM, current monitoring, open- and short-circuit detection,
        overcurrent and overtemperature protection, etc.

    usb_link/
        USB bridge with support for configuration and node firmware updates.

```
