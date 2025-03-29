import paho.mqtt.client as mqtt
from random import randint
import threading
import json
import time
# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected with result code {reason_code}")


def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))


def generateRandomData() -> dict[str:any]:
    data = {}
    data['external_temperature'] = randint(29,35)
    data["internal_temperature"] = randint(24,44)
    data['pressure'] = randint(9000,11500)
    data['methane'] = randint(0,18)
    data['ph'] = randint(0,27)
    data['slurry_level'] = randint(0,100)
    return data

def publish_data_periodically():
    """Background thread function to publish data every few seconds"""
    while True:
        try:
            # Generate random data
            data = generateRandomData()
            
            # Convert to JSON
            payload = json.dumps(data)
            
            # Publish to topic
            mqttc.publish("hello/world", payload)
            print(f"Published: {payload}")
            
            # Wait for 5 seconds before next publish
            time.sleep(3)
            
        except Exception as e:
            print(f"Error in publisher thread: {e}")
            time.sleep(3)  


publisher_thread = threading.Thread(target=publish_data_periodically, daemon=True)
publisher_thread.start()

mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqttc.on_connect = on_connect
mqttc.on_message = on_message


mqttc.connect("broker.emqx.io", 1883, 60)




# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
mqttc.loop_forever()