import paho.mqtt.client as mqtt
import csv
import argparse
import time

parser = argparse.ArgumentParser()
parser.add_argument("path", default='dane.txt')
args = parser.parse_args()

# This is the Publisher

client = mqtt.Client()
last_line = 0
while True:
  print('loop begining')
  last_line = 0
  with open(args.path, 'r') as csvfile:
    file_content = []
    for line in csvfile.readlines():
      file_content.append(line)
    
    if last_line < len(file_content):
      print("attempting connection, %d to send" % (len(file_content) - last_line))
      client.connect("localhost",1883,60)
      for l in range(last_line, len(file_content)):
        last_line += 1
        client.publish("test_mqtt", file_content[l]);
  print "sending succeded"
  time.sleep(3)
  client.disconnect();

