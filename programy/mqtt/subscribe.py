import paho.mqtt.client as mqtt
import csv
import argparse
import time

parser = argparse.ArgumentParser()
parser.add_argument("path", default='dane_max1202.txt')
args = parser.parse_args()
file=open(args.path, 'w')
# This is the Subscriber

def on_connect(client, userdata, flags, rc):
  print("Connected with result code "+str(rc))
  client.subscribe("test_mqtt")

def on_message(client, userdata, msg):
  file.write(msg.payload.decode())
  print(msg.payload.decode())  
  
client = mqtt.Client()
client.connect("192.168.1.100",1883,60)

client.on_connect = on_connect
client.on_message = on_message

client.loop_forever()
file.close()
