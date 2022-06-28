Library using: https://docs.espressif.com/projects/esp-idf/en/v4.3-beta3/esp32/

Steps for Setup: 

1. Install the ESP-IDF using above path, command: ./install.sh
2. Go to the esp-idf folder, run ./export.sh to activate idf.py
3. Go to occupancy sensing folder
4. Set ESP32-S2 as compiling target using command: idf.py set-target esp32s2 
5. Config Parameters with command: idf.py menuconfig
6. Inside config window, go to: 
     a. Component Config -> WiFi -> enable CSI, disable WiFi TX AMPTU, disable WiFi RX AMPDU, enable Fine Time Measurement. (press space to enable/disable)
     b. Component Config -> FREERTOS -> Tick Rate -> Set to 1000Hz
     c. Example configuration -> enable FTM Report logging (Show RTT values only)
     d. Save and Exit 



Performing the Experiment: 

1. Command: idf.py -p [port that ESP32S2 is using] flash monitor
2. Save the logging to files: Ctrl T, then Ctrl L to enable logging, when ended, Ctrl T and Ctrl L to disable and the log is autosaved.
3. *Exit the monitor mode: Ctrl ], exit at any time, this will disable logging and autosave the logged info to file. 


Save the logging to files: Ctrl T, then Ctrl L to enable logging, when ended, Ctrl T and Ctrl L to disable and the log is autosaved.
