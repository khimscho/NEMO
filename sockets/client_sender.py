## @file client_server.py
# @brief Simple python script to test the WiFi connection to the logger (in repeater mode)
#
# Copyright 2020 Center for Coastal and Ocean Mapping & NOAA-UNH Joint
# Hydrographic Center, University of New Hampshire.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
# OR OTHER DEALINGS IN THE SOFTWARE.

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
