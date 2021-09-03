#!/usr/bin/env python
# \file position_passthrough.py
# \brief Capture UDP packets from the network, and pass through to WIBL on the serial port

import socket
import serial

def report_buffer():
    while serport.in_waiting > 0:
        buf = serport.read(serport.in_waiting)
        print(buf.decode('UTF-8'))


serial_port = '/dev/cu.usbserial-AR0KBB3N'
serial_speed = 115200   # Default for WIBL 2.3
udp_port = 40181        # On HEALY, this is composite data on position, heading, speed, and depth

s = socket.socket(type = socket.SOCK_DGRAM)
s.bind(('', udp_port))

serport = serial.Serial(serial_port, serial_speed)
serport.write(b'help\r\n')
report_buffer()

serport.write(b'echo off\r\n')
serport.write(b'passthrough\r\n')
report_buffer()

try:
    while True:
        data = s.recv(250)
        print("Writing packet: %s" % (data))
        serport.write(data)
        report_buffer()

except KeyboardInterrupt:
    print("Shutting down.")
    serport.write(b'passthrough\r\n')
    serport.write(b'echo on\r\n')
    serport.flush()
    report_buffer()
