# python3.6

import random

from paho.mqtt import client as mqtt_client
from Crypto.Cipher import AES
from Crypto.Hash import BLAKE2s
import time
from datetime import datetime
import binascii



broker = 'localhost'
port = 1883
last_check = 0
current_time = 0
topic = "krishna_topic_1"
# generate client ID with pub prefix randomly
client_id = f'python-mqtt-1'
username = 'emqx'
password = 'public'

# For AES part
key=b'\x0A\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F'
cipher = AES.new(key, AES.MODE_ECB)

#For protection against replay attack
last_timestamp = None

def send_heartbeat(client):
    global last_check, current_time
    current_time = time.time()
    if(current_time - last_check >15):
        client.publish("acknowledge1", "Server is alive.")
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

def check_for_replay_attack(message):
    _, _, _, _, _, timeinfo= message.split(' ')
    timeinfo = datetime.fromisoformat(timeinfo.rstrip('Z'))

    last_timestamp = None
    try:
        last_message = open("sampleText.txt","r").readlines()[-1]
        _, _, _, _, _, last_timestamp= last_message.strip().split(' ')
        last_timestamp = datetime.fromisoformat(last_timestamp.rstrip('Z'))
    except Exception as e:
        pass
    if (not last_timestamp) or (timeinfo < last_timestamp):
        raise Exception("Received timestamp is earlier than last timestamp.")


def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        try:
            print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")
            encrypted, hash = msg.payload.decode().split(" Hash: ")
            encrypted_bytes = binascii.unhexlify(encrypted)

            message = cipher.decrypt(encrypted_bytes).decode().strip('\x04')
            print(message)

            hash_obj = BLAKE2s.new(digest_bits=256)
            hash_obj.update(message.encode())
            if hash_obj.hexdigest() != hash.lower():
                raise Exception("Hash does not match")
            
            check_for_replay_attack(message)
            #Send and heartbeat signal every 30 to ESP32 to make sure server is actively receving
            open("sampleText.txt","a").write(message + "\n")
            send_heartbeat(client)
        except Exception as e:
            print(f"Received exception as: {str(e)}")

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

