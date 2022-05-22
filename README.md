Library using: https://docs.espressif.com/projects/esp-idf/en/v4.3-beta3/esp32/

Steps for Setup: 

1. Install the ESP-IDF using above path, command: ./install.sh
2. Go to occupancy sensing folder
3. Set ESP32-S2 as compiling target using command: idf.py set-target esp32s2 
4. Config Parameters with command: idf.py menuconfig
5. Inside config window, go to Component Config -> WiFi -> enable CSI, disable WiFi TX AMPTU, disable WiFi RX AMPDU, enable Fine Time Measurement. (press space to enable/disable)
6. Go to Component Config -> FREERTOS -> Tick Rate -> Set to 1000Hz
7. Exit and save everything. 

For every time flashing the device: 

1. Go to the esp-idf folder, run ./export.sh to activate idf.py


Performing the Experiment: 

1. Command: idf.py -p [port that ESP32S2 is using] flash monitor
2. Save the logging to files: Ctrl T, then Ctrl L to enable logging, when ended, Ctrl T and Ctrl L to disable and the log is autosaved.
3. *Exit the monitor mode: Ctrl ], exit at any time, this will disable logging and autosave the logged info to file. 
