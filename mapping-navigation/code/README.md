# mapping-navigation

# Team Members
- LEONG WENG YAN IVAN (1900688)
- CHEW CHYOU KEAT LIONEL (2101371)
- LAU HUI QI (2102823)
- LAU JUN XIANG (2100582)


# How it works (Grid mapping algorithim)
1. Mapping algorithim will get input from ultrasonic sensors on the left,
right, and front to detact if there is a wall or clear path. (will be using user input to simulate)

2. When there is an open path, it will move by one grid towards that direction.

3. Mapping algorithim will always prioritize right turns first, followed by left, and lastly moving straight.

4. When more then 2 paths are available, e.g left and straight, it will take the direction that has the highest
priority. It will also avoid grids that had been visited before.

5. The grids that did not get prioritized will be added to the multi move list, provided that they are unvisited.

6. Grids in the multi move list will be visited later, and it will continue the mapping algorithim.

7. Eventually, the algorithim will reach either a dead end or a grid that had been visited before. This is
when it will find a path to a grid in the multi move list.

8. Once at the multi move grid, it will continue the mapping algorithim to find more paths.

9. The algorithim will run till it is back at visited grid with no more girds in the multimove list. This
would indicate that the mapping is complete.



# Steps to run
1. Connect the Pico to the PC while ensuring the BOOTSEL button is pressed. The RPI-RP2 drive will be opened.

2. In build/main, drag the main.uf2 file into the RPI-RP2 drive to flash it to pico.

3. Open a terminal emulator with serial comunitcaions. (e.g putty)

4. In the terminal setup, enter serial COM number of pico and set speed/baudrate to 115200 and connect

5. Interact with program through the terminal.