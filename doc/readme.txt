# Ripping

Tools required:
- https://docs.microsoft.com/en-us/sysinternals/downloads/debugview
- https://github.com/vir91/driverspy
- An XP SP3 machine.

Steps:
Have XP SP3 machine ready, with ess 1969 card.
Change PH_TARGET_MODULE_NAME in DriverSpy.h to L"es1969.sys" and compile.
Ready DebugView and enable "Capture Kernel", disable "Capture Win32".
Capturing kernel debug might require restarting DebugView.
Use a driver loader to manage DriverSpy.

DriverSpy is rather unstable, so record quickly, save DebugView log quickly,
and stop+delete DriverSpy after being done.
As a precaution, disable and re-enable soundcard from device manager.

# Instrument bank structure
- Each instrument seems to be 28 bytes long.

# Hardware programming info

TODO