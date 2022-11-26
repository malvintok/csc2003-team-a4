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

# Connecting to hotspot
lcd.print('Connecting to hotspot', 0, 0, 0xffffff)
wifiCfg.doConnect('GalaxyNote10', 'qeyz5236')

# Connecting to mqtt broker
m5mqtt = M5mqtt('', 'broker.emqx.io', 1883, '', '', 300)

while not (wifiCfg.wlan_sta.isconnected()):
  wait(2)

lcd.print('Connected to hotspot', 0, 0, 0xffffff)

# Declaring GP33 as TX and GP32 as RX for UART
uart1 = machine.UART(1, tx=33, rx=32)

# Initiating UART connection
uart1.init(115200, bits=8, parity=None, stop=1)

# triggered everytime there is a new publish in flask/mapcoordinates
# def fun_flask_mapcoordinates_(topic_data):
#   # global params
#   global coords
#   coords = str(topic_data)
#   lcd.print(coords, 0, 0, 0xffffff)
#   uart1.write(coords)
#   lcd.clear()
#   pass

# m5mqtt.subscribe(str('flask/mapcoordinates'), fun_flask_mapcoordinates_)

m5mqtt.start()

lcd.clear()

data = ''
queue = []

while True:
  # check for any uart data received
  if(uart1.any()):
    
    lcd.clear()
    lcd.print('Reading data.', 0, 0, 0xffffff)

    # reading data from pico
    data = (uart1.readline().decode())
    # separating by delimiter
    queue = data.split('#')
    
    # separating data based on id (refer to comms.c in the indiv uart_puts methods to double check)
    for i in queue:
      if(i[0:1] == '1'):    
        # sending data to mqtt broker -> web app subscribes to topics to get data   
        m5mqtt.publish(str('pico/wheelvelocityl'), str(i[1:]), 0)
      if(i[0:1] == '2'):
        m5mqtt.publish(str('pico/barcode'), str(i[1:]), 0)
      if(i[0:1] == '3'):
        m5mqtt.publish(str('pico/distancefromsensor'), str(i[1:]), 0)
      if(i[0:1] == '4'):
        m5mqtt.publish(str('pico/heightofhump'), str(i[1:]), 0)
      if(i[0:1] == '5'):
        m5mqtt.publish(str('pico/mapcoordinates'), str(i[1:]), 0)
      if(i[0:1] == '6'):
        m5mqtt.publish(str('pico/barcodecoordinates'), str(i[1:]), 0)
      if(i[0:1] == '7'):
        m5mqtt.publish(str('pico/humpcoordinates'), str(i[1:]), 0)
      if(i[0:1] == '8'):    
        m5mqtt.publish(str('pico/wheelvelocityr'), str(i[1:]), 0)
  
    wait_ms(500)

  else:
    lcd.clear()
    lcd.print('Not reading data.', 0, 0, 0xffffff)
    pass