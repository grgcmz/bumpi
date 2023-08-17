import socket
import threading

BUFF_SIZE = 65536
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET,socket.SO_RCVBUF,BUFF_SIZE)

ip ="localhost"
port = 1235

#binding ip and port number
s.bind((ip,port))

#receives packet from sender and show print
def recv():
    while True:
        packet,_ = s.recvfrom(BUFF_SIZE)
        print(packet.decode('utf-8'))

       

#multithreading
x1 = threading.Thread( target=recv )

x1.start()