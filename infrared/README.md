# CSC2003 A4 Sub-Group: Encoder/Barcode

|  Team Members                 | Student ID    |
|---                            |---       |
|  Lee Jun Hao Jeff             | 2101596  |
|  Tan Xin Jie                  | 2101968  |
|  Tang Guan You                | 2101672  |

---
## Set-Up Guide (Build)
1. Use Developer Command Prompt and change directory to the folder containing Barcodencoder.c and CMakeList.txt
2. Enter Visual Studio from Command Prompt with the command
```
code .
```
3. Navigate to Barcodencodeer.c in Visual Studio
4. Ensure cmake variant is 'Debug' or 'Release'
5. Ensure compiler is GCC 10.3.1 arm-non-eabi
6. Click on 'Build' and build folder will be generated
7. Navigate to '\build\barcodencoder'
---
## Put build into Pico
1. Connect Pico Serially to PC, ensure button of Pico is pressed as the USB is slotted into the PC
2. Folder of Pico Storage will be opened
3. In '\build\barcodencoder', put barcodencoder.uf2 into Pico Storage
4. Connect to Serial Console (PuTTY) to view results
---
## Demonstration
1. Connect to serial console (PuTTY)
2. Start from a black background and move at a constant speed with till the end of the barcode
3. Barcode results will be displayed on screen with successive scanning
4. Concurrently, the speed of the car will be displayed in cm/s on screen