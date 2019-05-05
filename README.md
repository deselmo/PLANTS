# P.L.A.N.T.S.

> Plants Laborsaving Automated Nurturing Tracking and Safeguarding


# HOW TO DEBUG

First of all build the project to obtain the library files

Copy this to the file ```./yotta_modules\nrf51-sdk\source\supress-warnings.cmake```

the following snippeet

```
message("omit frame pointer for bootloader startup asm")

 if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	set_source_files_properties(
		"${CMAKE_CURRENT_LIST_DIR}/nordic_sdk/components/libraries/bootloader_dfu/bootloader_util.c"
		PROPERTIES COMPILE_FLAGS -fomit-frame-pointer)
endif()
```

Then you need to remove from the downloaded source every source of the bluetooth

so remove the following paths 
```./yotta_modules/microbit-dal/inc/bluetooth```
```./yotta_modules/microbit-dal/source/bluetooth```
and comment the lines that starts with **bluetooth** in the file ```./yotta_modules/microbit-dal/source/CMakeLists.txt```

```
    # "bluetooth/MicroBitAccelerometerService.cpp"
    # "bluetooth/MicroBitBLEManager.cpp"
    # "bluetooth/MicroBitButtonService.cpp"
    # "bluetooth/MicroBitDFUService.cpp"
    # "bluetooth/MicroBitEddystone.cpp"
    # "bluetooth/MicroBitEventService.cpp"
    # "bluetooth/MicroBitIOPinService.cpp"
    # "bluetooth/MicroBitLEDService.cpp"
    # "bluetooth/MicroBitMagnetometerService.cpp"
    # "bluetooth/MicroBitTemperatureService.cpp"
    # "bluetooth/MicroBitUARTService.cpp"
    # "bluetooth/MicroBitPartialFlashingService.cpp"
```

then in the file ```yotta_modules\microbit-dal\source\drivers\MicroBitRadio.cpp```

comment the line ```#include "MicroBitBLEManager.h"```
change the code of the function ```setTransmitPower```
to
```
/*if (power < 0 || power >= MICROBIT_BLE_POWER_LEVELS)
        return MICROBIT_INVALID_PARAMETER;

    NRF_RADIO->TXPOWER = (uint32_t)MICROBIT_BLE_POWER_LEVEL[power];
    */
    NRF_RADIO->TXPOWER = (uint32_t)0;
    return MICROBIT_OK;
```

then in the file ```yotta_modules\microbit\inc\MicroBit.h```

comment the lines 
```#include "MicroBitBLEManager.h"```
```MicroBitBLEManager		    bleManager;```
```BLEDevice                   *ble;```

change the body of the function ```reset```
to
```
    /*
    if(ble && ble->getGapState().connected) {

        // We have a connected BLE peer. Disconnect the BLE session.
        ble->gap().disconnect(Gap::REMOTE_USER_TERMINATED_CONNECTION);

        // Wait a little while for the connection to drop.
        wait_ms(100);
    }
    */

    microbit_reset();
```

then in the file ```yotta_modules\microbit\source\MicroBit.cpp```

comment the lines
```bleManager(storage),```
```ble(NULL)```
and remove the ```,``` after the command ```radio()```

then comment which is after 
```
#if CONFIG_ENABLED(MICROBIT_HEAP_REUSE_SD)
            microbit_create_heap(MICROBIT_SD_GATT_TABLE_START + MICROBIT_SD_GATT_TABLE_SIZE, MICROBIT_SD_LIMIT);
#endif
```

```
/*if (!ble)
            {
                bleManager.init(getName(), getSerial(), messageBus, true);
                ble = bleManager.ble;
            }

            // Enter pairing mode, using the LED matrix for any necessary pairing operations
            bleManager.pairingMode(display, buttonA);
            */
```


To compile the file execute ```$> yt build --debug```

Then to debug you need to have in the directory ```.vscode``` the file ```launch.json``` with the following in it

```
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Microbit Debug",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceRoot}\\build\\bbc-microbit-classic-gcc\\source\\plants-combined.hex",
            "args": [],
            "stopAtEntry": true,
            "cwd": "${workspaceRoot}",
            "environment": [],
            "externalConsole": true,
            "debugServerArgs": "--persist -t nrf51 -bh -r",
            "serverLaunchTimeout": 20000,
            "filterStderr": true,
            "filterStdout": false,
            "serverStarted": "GDB\\ server\\ started",
            "logging": {
                "moduleLoad": false,
                "trace": true,
                "engineLogging": true,
                "programOutput": true,
                "exceptions": false
            },
            "windows": {
                "MIMode": "gdb",
                "MIDebuggerPath": "C:\\yotta\\gcc\\bin\\arm-none-eabi-gdb.exe",
                "debugServerPath": "C:\\yotta\\workspace\\Scripts\\pyocd-gdbserver.exe",
                "debugServerArgs": "--persist -t nrf51 -bh -r",
                "setupCommands": [
                    { "text": "-environment-cd ${workspaceRoot}\\build\\bbc-microbit-classic-gcc\\source" },
                    { "text": "-target-select remote localhost:3333", "description": "connect to target", "ignoreFailures": false },
                    { "text": "-interpreter-exec console \"monitor reset\"", "ignoreFailures": false },
                    { "text": "-interpreter-exec console \"monitor halt\"", "ignoreFailures": false },
                    { "text": "-interpreter-exec console \"monitor soft_reset_halt\"", "ignoreFailures": false },
                    { "text": "-file-exec-file ./plants-combined.hex", "description": "load file", "ignoreFailures": false},
                    { "text": "-file-symbol-file ./plants", "description": "load synbol file", "ignoreFailures": false},
                ]
            }
        }
    ]
}
```

This works for windows, for linux is necessary to change the path to the debugger and to the server then it should work