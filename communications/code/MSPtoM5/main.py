from m5stack import *
from m5ui import *
from uiflow import *
import wifiCfg
import time
from m5mqtt import M5mqtt
import ntptime
import json

# PINS M5Stick
# GP33 - TX
# GP32 - RX

coords = ''

lcd.setRotation(1)

# Declaring GP33 as TX and GP32 as RX for UART
uart1 = machine.UART(1, tx=33, rx=32)

# Initiating UART connection
uart1.init(9600, bits=8, parity=None, stop=1)


lcd.clear()

data = ''

while True:
  if(uart1.any()):
    
    lcd.clear()
    lcd.print('Reading data.', 0, 0, 0xffffff)

    # reading data from pico
    data = (uart1.readline().decode())
    lcd.print(data, 0, 0, 0xffffff)
  else:
    lcd.clear()
    lcd.print('Not reading data.', 0, 0, 0xffffff)
    pass
    