import paho.mqtt.client as mqtt

broker_address = "localhost"
port = 1883
topic = "camera/take_photo"

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

client = mqtt.Client("Python_Publisher")
client.on_connect = on_connect

client.connect(broker_address, port, 60)

# Publishing a message to ask the ESP32 to take a photo
client.publish(topic, "TAKE_PHOTO")

# Disconnect from the broker
client.disconnect()
