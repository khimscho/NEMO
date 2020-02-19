import socket

#address = "192.168.4.1"
address = "127.0.0.1"
port = 25443;

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((address, port))
s.listen()

reply = 0

try:
    while True:
        (clientsocket, address) = s.accept()
        
        buffer = clientsocket.recv(1024)
        print("Received " + str(len(buffer)) + " bytes: \"" + str(buffer) + "\"")
        
        buffer = "Reply " + str(reply)
        reply += 1
        
        sent = clientsocket.send(bytearray(buffer, "utf8"))
        print("Sent " + str(sent) + " bytes: \"" + buffer + "\"")

except KeyboardInterrupt:
    print("Shutting down")
