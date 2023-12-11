# python3.6

import random

from paho.mqtt import client as mqtt_client
from Crypto.Cipher import AES
from Crypto.Hash import BLAKE2s
import time



broker = '192.168.1.40'
port = 1883
last_check = 0
current_time = 0
topic = "krishna_topic_2"
# generate client ID with pub prefix randomly
client_id = f'python-mqtt-{random.randint(0, 100)}'
username = 'emqx'
password = 'public'

# For AES part
key=b'\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F'
cipher = AES.new(key, AES.MODE_ECB)

def send_heartbeat(client):
    global last_check, current_time
    current_time = time.time()
    if(current_time - last_check >15):
        client.publish("acknowledge", "Server is alive.")
        last_check = current_time

def connect_mqtt() -> mqtt_client:
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)

    client = mqtt_client.Client(client_id)
    client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client


def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        if (len(msg.payload) % 16 == 0):
            print(f"Received `{cipher.decrypt(msg.payload).decode()}` from `{msg.topic}` topic")
            decoded_message = cipher.decrypt(msg.payload).decode().strip('\0')
            message, hash = decoded_message.split(" Hash: ")

            hash_obj = BLAKE2s.new(digest_bits=256)
            hash_obj.update(message.encode())
            if hash_obj.hexdigest() == hash.lower():
                open("sampleText2.txt","a").write(message + "\n")
            else:
                raise Exception("Hash does not match")
            #Send and heartbeat signal every 30 to ESP32 to make sure server is actively receving
            send_heartbeat(client)

    client.subscribe(topic)
    client.on_message = on_message

def run():
    client = connect_mqtt()
    subscribe(client)
    #Send and heartbeat signal every 30 to ESP32 to make sure server is actively receving
    send_heartbeat(client)
    client.loop_forever()


if __name__ == '__main__':
    run()

