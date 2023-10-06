import paho.mqtt.client as mqtt

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe("test")

def on_message(client, userdata, msg):
    print(f'Topic: {msg.topic} - Message: {msg.payload.decode()}')

def main():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect("192.168.129.132", 1883, 60)

    client.loop_forever()

if __name__ == "__main__":
    main()
