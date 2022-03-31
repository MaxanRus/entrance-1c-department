from socket import *

host = '127.0.0.1'
port = 7777
addr = (host,port)

udp_socket = socket(AF_INET, SOCK_DGRAM)

data = "hello world"

data = str.encode(data)
while True:
    udp_socket.sendto(data, addr)

udp_socket.close()
