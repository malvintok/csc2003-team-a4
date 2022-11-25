from flask import Flask, jsonify
from flask import sessions
from flask import request
from flask import render_template
from flask import redirect, url_for
from definition import *
from flask_mqtt import Mqtt
from flask_socketio import SocketIO
from pyamaze import maze

app = Flask(__name__)
ALLOWED_EXTENSIONS = set(['jpg', 'jpeg'])
session = {}

'''
for int routes
@app.route('/<int:id>')
def func(id):
    print(id)
'''

@app.route('/')
@app.route('/index')
def index():
    return render_template("data.html")


@app.route('/map')
def map():
    return render_template("map.html")

@app.route('/data')
def getData():
    return render_template("data.html")


#-----------------------------------------------------------------------------------------------------------------------#

app.config['MQTT_BROKER_URL'] = 'broker.emqx.io'
app.config['MQTT_BROKER_PORT'] = 1883
app.config['MQTT_CLIENT_ID'] = ''
app.config['MQTT_USERNAME'] = ''
app.config['MQTT_PASSWORD'] = ''
app.config['MQTT_KEEPALIVE'] = 300  # Set KeepAlive time in seconds
app.config['MQTT_TLS_ENABLED'] = False  # If your server supports TLS, set it True

# declaring mqtt routes
wheelvelocityl = 'pico/wheelvelocityl' #wheel velocity of left wheel
wheelvelocityr = 'pico/wheelvelocityr' #wheel velocity of right wheel
barcode = 'pico/barcode' #barcode information
distancefromsensor = 'pico/distancefromsensor' #distance of obstruction from the sensor
heightofhump = 'pico/heightofhump' 
mapcoordinates = 'pico/mapcoordinates' #coordinate of map in (x,y,north,south,east,west) format
barcodecoordinates = 'pico/barcodecoordinates' #coordinate of where the barcode ends (x,y)
humpcoordinates = 'pico/humpcoordinates'
coordfromFlask = 'flask/mapcoordinates' #!!fail implementation should be remove!!?

# initializing mqtt and socket
mqtt_client = Mqtt(app)
socketio = SocketIO(app)

# subscribe to topics on connect
@mqtt_client.on_connect()
def handle_connect(client, userdata, flags, rc):
   if rc == 0:
       print('Connected successfully')
       mqtt_client.subscribe(wheelvelocityl)
       mqtt_client.subscribe(wheelvelocityr)
       mqtt_client.subscribe(barcode)
       mqtt_client.subscribe(distancefromsensor)
       mqtt_client.subscribe(heightofhump)
       mqtt_client.subscribe(mapcoordinates)
       mqtt_client.subscribe(barcodecoordinates)
       mqtt_client.subscribe(humpcoordinates)
       mqtt_client.subscribe(coordfromFlask)
   else:
       print('Bad connection. Code:', rc)

# storing mqtt data in data variable for display
@mqtt_client.on_message()
def handle_mqtt_message(client, userdata, message):
    data = dict(topic=message.topic,payload=message.payload.decode()) 
    # emit a mqtt_message event to the socket containing the message data using socket.on in the html page
    socketio.emit('mqtt_message', data=data)
    # debugging purposes
    print('Received message on topic: {topic} with payload: {payload}'.format(**data))

@mqtt_client.on_log()
def handle_logging(client, userdata, level, buf):
    print(level, buf)


# -------------- MQTT Routes --------------------------
# publishing messages to broker
@app.route('/publish', methods=['POST'])
def publish_message():
   coord = request.form.get("coord")
   mqtt_client.publish(coordfromFlask, coord)
   return render_template('data.html')

# -------------- MQTT Routes --------------------------

if __name__  == "__main__":
    socketio.run(app, host='127.0.0.1', port=5000, use_reloader=True, debug=True)
  




