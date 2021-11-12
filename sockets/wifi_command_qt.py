## @file wifi_command_qt.py
# @brief Python Qt application to provide a simple console interface to the logger's WiFi interface
#
# This code uses the Qt widget set to make a very simple command-line
# interface to the logger's WiFi interface (assuming it's turned on
# through either serial or BLE interfaces with "wireless on"), so that
# the user can test the interface is working, and interrogate the data
# files on the logger.  The code here also allows for the files on the
# logger to be transferred to the local computer, and has a built-in
# command ("translate") to use the data parser code to inspect the contents
# of the file.  It's a little shonky, and cobbled together from multiple
# examples and sources, but it sort of works, which is good enough for
# these purposes.
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

from PyQt5 import QtWidgets, uic
from PyQt5.QtWidgets import QMessageBox, QDialog, QFileDialog
from PyQt5.QtCore import QThread, QObject, pyqtSignal, pyqtSlot
import sys
import os
import socket
import threading
import time
import boto3
import uuid

sys.path.append("../DataParser")

import LoggerFile


class CaptureReturn(QObject):
    messageReady = pyqtSignal(str)
    
    def __init__(self):
        QObject.__init__(self)
        
    @pyqtSlot()
    def capture(self):
        global binary_mode
        global server_sock
        while True:
            try:
                first_byte = server_sock.recv(1)
                if binary_mode:
                    # In binary mode, we can't grab sentences and emit them as they come, since there aren't natural breaks
                    # and in any case we don't want to spam the text record with random binary data.  The goal here is to
                    # report how many bytes are expected, show the first few packets, and then confirm that it's done.  We
                    # rely on the logger sending a length word as the first piece of data.
                    transfer_size_buffer = first_byte + server_sock.recv(3)
                    transfer_size = int.from_bytes(transfer_size_buffer, "little")
                    buffer = "Binary transfer: " + str(transfer_size) + " bytes to \"" + binary_filename + "\".\n"
                    self.messageReady.emit(buffer)
                    f = open(binary_filename, 'wb')
                    bytes_transferred = 0
                    for b in range(transfer_size):
                        in_byte = server_sock.recv(1)
                        f.write(in_byte)
                        bytes_transferred += 1
                    f.close()
                    buffer = "Transferred " + str(bytes_transferred) + " bytes total.\n"
                    self.messageReady.emit(buffer)
                else:
                    # In non-binary mode, the data that we're going to get is just sentences of ASCII text, and therefore
                    # we gather data into a buffer until we see a newline, and then emit for the record.  We get a chance
                    # to convert modes at the end of each sentence.
                    buffer = first_byte.decode("utf8")
                    sentence_complete = False
                    while not sentence_complete:
                        in_byte = server_sock.recv(1)
                        ch = in_byte.decode("utf8")
                        if ch != '\r':
                            buffer += ch
                        if ch == '\n':
                            sentence_complete = True
                    self.messageReady.emit(buffer)
            except BlockingIOError:
                pass
            except:
                e = sys.exc_info()[0]
                print("Rx Exception: " + str(e))


class Window(QtWidgets.QMainWindow):
    
    def __init__(self):
        super(Window, self).__init__()
        uic.loadUi('gui.ui', self)
        self.commandLine.returnPressed.connect(self.doCommand)
        self.startConnection.clicked.connect(self.onStartConnection)
        self.serverAddress.setText("192.168.4.1")
        self.serverPort.setText("25443")

        # We start the capture thread that grabs output from the server, and buffers for display
        self.connection_started = False
        self.obj = CaptureReturn()
        self.rx_thread = QThread()
        self.obj.messageReady.connect(self.outputMessageReady)
        self.obj.moveToThread(self.rx_thread)
        self.rx_thread.started.connect(self.obj.capture)

    @pyqtSlot()
    def onStartConnection(self):
        global server_sock
        server_address = self.serverAddress.text()
        server_port = self.serverPort.text()
        address_string = server_address + "/" + server_port
        print("Attempting to start server connection on " + address_string + "\n")
        try:
            server_sock = socket.create_connection((server_address, server_port), 5.0)
            server_sock.settimeout(None)
            self.rx_thread.start()
            self.connection_started = True
            buffer = "Server connected on " + address_string + "\n"
            self.updateRecord(buffer)
        except:
            buffer = "Server failed to connect on " + address_string + "\n"
            self.updateRecord(buffer)
        
    @pyqtSlot()
    def doCommand(self):
        global server_sock
        cmd = self.commandLine.text()
        self.commandLine.setText("")
        self.updateRecord("$ " + cmd + "\n")
        # We start by parsing any local commands (i.e., that only exist in the GUI), then attempt
        # to parse out a command that's going to the logger.  The only intervention that is done is
        # to intercept "transfer" commands so that we can synthesise a file name for the output, and
        # set the transfer into binary mode so we don't spam the record with random binary data.
        if cmd.startswith("binary"):
            try:
                mode = cmd.split(' ')[1]
                if mode == "on":
                    set_transfer_mode(True)
                else:
                    set_transfer_mode(False)
            except:
                buffer = "Failed\nSyntax: binary on|off\n"
                self.updateRecord(buffer)
        elif cmd.startswith("translate"):
            try:
                file_number = cmd.split(' ')[1]
                file_name = "wibl-raw." + str(file_number) + ".wibl"
                self.translateFile(file_name)
            except:
                buffer = "Failed\nSyntax: translate <file number>\n"
                self.updateRecord(buffer)
        elif cmd.startswith("upload"):
            try:
                file_number = cmd.split(' ')[1]
                file_name = "wibl-raw." + str(file_number) + ".wibl"
                self.uploadFile(file_name)
            except:
                buffer = "Failed\nSyntax: upload <file number>\n"
                self.updateRecord(buffer)
        else:
            # We get to here only if there's a command to send to the
            # logger.  We can only do that, however, if the connection
            # has actually been started.
            if self.connection_started:
                try:
                    if cmd.startswith("transfer"):
                        set_transfer_mode(True)
                        file_number = cmd.split(' ')[1]
                        file_name = "wibl-raw." + str(file_number) + ".wibl"
                        set_transfer_filename(file_name)
                    else:
                        set_transfer_mode(False)
                    server_sock.send(cmd.encode("utf8"))
                except:
                    buffer = "Failed: command not accepted on logger\n"
                    self.updateRecord(buffer)
            else:
                buffer = "Can't run commands if the server isn't connected!\n"
                self.updateRecord(buffer)
                self.commandLine.setText("")
            
    @pyqtSlot(str)
    def outputMessageReady(self, message):
        self.updateRecord(message)
        
    def translateFile(self, filename):
        # We should probably put this into another thread, so that it can churn through this
        # in the background without stalling the GUI.
        file = open(filename, "rb")
        packet_count = 0
        source = LoggerFile.PacketFactory(file)
        while source.has_more():
            pkt = source.next_packet()
            if pkt is not None:
                buffer = str(pkt) + "\n"
                self.updateRecord(buffer)
                packet_count += 1
        buffer = "Found " + str(packet_count) + " packets total\n"
        self.updateRecord(buffer)
        
    def uploadFile(self, filename):
        # Upload to the S3 bucket for incoming data, ready to be timestamped, converted
        # to GeoJSON, and sent to DCDB
        s3 = boto3.resource('s3')
        dest_bucket = s3.Bucket('csb-upload-ingest-bucket')
        try:
            obj_key = str(uuid.uuid4())
            dest_bucket.upload_file(Filename=filename, Key=obj_key)
            buffer = "Upload successful\n"
        except:
            buffer = "Failed to upload file to CSB ingest bucket\n"
        self.updateRecord(buffer)

    def updateRecord(self, message):
        self.consoleOutput.setText(self.consoleOutput.toPlainText() + message)
        self.consoleOutput.verticalScrollBar().setValue(self.consoleOutput.verticalScrollBar().maximum())

def set_transfer_mode(flag):
    global binary_mode
    binary_mode = flag
    
def set_transfer_filename(name):
    global binary_filename
    binary_filename = name
    
server_sock = socket.socket()

binary_mode = False
binary_filename = ""

app = QtWidgets.QApplication([])
win = Window()
win.show()
sys.exit(app.exec())
