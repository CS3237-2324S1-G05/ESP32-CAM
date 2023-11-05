import paho.mqtt.client as mqtt

broker_address = "localhost"
port = 1883
topic = "camera/take_photo"

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe(topic)

# The callback for when a PUBLISH message is received from the ESP32.
def on_message(client, userdata, msg):
    print(f"Received message on {msg.topic}")
    with open("received_photo.jpg", "wb") as photo:
        photo.write(msg.payload)

client = mqtt.Client("Python_Subscriber")
client.on_connect = on_connect
client.on_message = on_message

client.connect(broker_address, port, 60)

# Subscribe to the topic
client.subscribe(mqtt_topic)
# Blocking call that processes network traffic, dispatches callbacks and handles reconnecting.
client.loop_forever()
