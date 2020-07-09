## @file server_reflector.py
# @brief Simple Python code to reflect any messages coming in on a socket (WiFi testing)
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
