# CSC2003 A4 Sub-Group: Communications

|  Team Members                 | Student ID |
|---                            |---      |
|  Abdullah Abdul Rahman        | 2100972 |
|  Amiir Hamzah                 | 2101526 |
|  Nicole Ng                    | 2102552 |

---
## Set Up Guide

## Pico 
1. Connect the Pico to the PC while ensuring the BOOTSEL button is pressed. The RPI-RP2 drive will be opened.
2. In comms/build/comms, drag the comms.uf2 file into the RPI-RP2 drive to run the pico code.

## M5StickC Plus (Ensure that FTDI Driver in installed and the M5 firmware is burned with UIFlow_StickC_Plus)
(https://docs.m5stack.com/en/quick_start/m5stickc_plus/uiflow)
1. Prerequisites: In Visual Studio Code install the vscode-m5stack-mpy extension.
2. Connect the M5Stick to the PC in USB mode.
3. At the bottom of Visual Studio, select "Add M5Stack" then select the correct com port number. 
4. On the left, there will be a tab named "M5Stack Device". 
5. Select the tab and look for main.py
6. Copy the code from CSC2003-main/CSC2003Dashboard/M5StickCPlus/main.py into the main.py of the m5stick.
7. At line 20 at wifiCfg replace the details with 
7. We can now run the M5 code through visual studio by selecting the darkened play button at the top right
8. Or we can run the m5 code wirelessly by pressing the left button on the M5 once, and then the right button until we see .py on the screen which indicates the main.py in the M5stickC will start running.


## Flask Web Application
1. Prerequisites: Install the required libraries (flask, flask_mqtt, socket.io)
2. In CSC2003-main/CSC2003Dashboard, run main.py
2. Head to 127.0.0.1:5000
3. Select RoboCar to view data, and Map to view the plotting of map

