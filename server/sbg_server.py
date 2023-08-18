import socket
import threading
import struct
import matplotlib
import matplotlib.pyplot as plt


BUFF_SIZE = 65536
format_string = 'HBBBBBIdddd'
sensor_data = ""

def receive_sensor_data():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF,BUFF_SIZE)
    matplotlib.use('WebAgg')

    ip ="0.0.0.0"
    port = 1235

    #binding ip and port number
    s.bind((ip,port))

    #multithreading
    x1 = threading.Thread(target=recv(s))

    x1.start()

# receives packet from sender and send them back to server for display
def recv(s):
    while True:
        global sensor_data
        packet,_ = s.recvfrom(struct.calcsize(format_string))
        unpacked_data = struct.unpack(format_string, packet)
        year, month, day, hour, minute, second, nanosecond, euler, altitude, latitude, longitude = unpacked_data
    
        # Create a plot
        plt.figure(figsize=(10, 6))
        plt.plot([euler[0], euler[1], euler[2]], marker='o')
        plt.title("Euler Angles")
        plt.xlabel("Axis")
        plt.ylabel("Angle (degrees)")
        plt.xticks(range(3), ['X', 'Y', 'Z'])
    
        # Display the plot
        plt.show()

if __name__ == "__main__":
   receive_sensor_data()