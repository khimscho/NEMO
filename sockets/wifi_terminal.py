import socket

address = "192.168.4.1"
port = 25443;

message = 0

try:
    s = socket.create_connection((address, port))
    while True:
        buffer = input()
        sent = s.send(bytearray(buffer, "utf8"))

        sentence = False
        buffer = ""
        while not sentence:
            byte = s.recv(1)
            ch = byte.decode("utf8")
            if (ch == '\n'):
                print("RX: " + str(buffer))
                buffer = ""
                sentence = True
            else:
                if (ch != '\r'):
                    buffer += ch

except KeyboardInterrupt:
    print("Shutting Down")
