import socket

address = "192.168.4.1"
#address = "127.0.0.1"
port = 25443;

message = 0

try:
    s = socket.create_connection((address, port))
    while True:
        buffer = "Message " + str(message) + "\n"
        message += 1
        sent = s.send(bytearray(buffer, "utf8"))
        print("Sent " + str(sent) + " bytes: \"" + buffer + "\"")
        
        sentence = False
        buffer = ""
        while not sentence:
            byte = s.recv(1)
            ch = byte.decode("utf8")
            if (ch == '\n'):
                print("Received " + str(len(buffer)) + " bytes: \"" + str(buffer) + "\"")
                buffer = ""
                sentence = True
            else:
                if (ch != '\r'):
                    buffer += ch

except KeyboardInterrupt:
    print("Shutting Down")
