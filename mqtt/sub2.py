# python3.6

import random

from paho.mqtt import client as mqtt_client
from Crypto.Cipher import AES



broker = 'broker.emqx.io'
port = 1883
topic = "krishna_topic_2"
# generate client ID with pub prefix randomly
client_id = f'python-mqtt-{random.randint(0, 100)}'
username = 'emqx'
password = 'public'

# For AES part
key=b'\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F'
cipher = AES.new(key, AES.MODE_ECB)


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
        # print(cipher.decrypt(msg.payload).decode())
        if (len(msg.payload) % 16 == 0):
            print(f"Received `{cipher.decrypt(msg.payload).decode()}` from `{msg.topic}` topic")
            open("sampleText2.txt","a").write(cipher.decrypt(msg.payload).decode().strip('\0') + "\n")

    client.subscribe(topic)
    client.on_message = on_message


def run():
    client = connect_mqtt()
    subscribe(client)
    client.loop_forever()


if __name__ == '__main__':
    run()

