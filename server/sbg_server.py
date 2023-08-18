import socket
import threading

BUFF_SIZE = 65536
sensor_data = ""

def receive_sensor_data():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET,socket.SO_RCVBUF,BUFF_SIZE)

    ip ="localhost"
    port = 1235

    #binding ip and port number
    s.bind((ip,port))

    #multithreading
    x1 = threading.Thread(target=recv(s))

    x1.start()

# receives packet from sender and send them back to server for display
def recv(s):
    while True:
        packet,_ = s.recvfrom(BUFF_SIZE)
        global sensor_data
        sensor_data = (packet.decode('utf-8'))
        print(sensor_data)

if __name__ == "__main__":
   receive_sensor_data()