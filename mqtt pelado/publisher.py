import paho.mqtt.client as mqtt
import curses

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

def main(stdscr):
    # Properly configure the curses environment
    curses.curs_set(0)
    stdscr.nodelay(1)
    stdscr.timeout(100)

    client = mqtt.Client()
    client.on_connect = on_connect

    client.connect("192.168.129.132", 1883, 60)

    while True:
        c = stdscr.getch()
        if c != -1:
            if c == curses.KEY_UP:
                client.publish("test", "UP")
            elif c == curses.KEY_DOWN:
                client.publish("test", "DOWN")
            elif c == curses.KEY_RIGHT:
                client.publish("test", "RIGHT")
            elif c == curses.KEY_LEFT:
                client.publish("test", "LEFT")
            elif c == ord('q'):  # Press 'q' to quit
                break

    curses.endwin()  # Restore the terminal to its original state

if __name__ == "__main__":
    curses.wrapper(main)
