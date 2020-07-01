from PyQt5 import QtWidgets, uic
from PyQt5.QtWidgets import QMessageBox, QDialog, QFileDialog
from PyQt5.QtCore import QThread, QObject, pyqtSignal, pyqtSlot
import sys
import os
import socket
import threading
import time


class CaptureReturn(QObject):
    messageReady = pyqtSignal(str)
    def __init__(self):
        QObject.__init__(self)
        self.binary_mode = False
        self.binary_filename = ""

    @pyqtSlot()
    def set_binary_mode(self, flag):
        self.binary_mode = flag
        print("Binary transfer mode set to: " + flag)
        
    @pyqtSlot()
    def set_binary_filename(self, name)
        self.binary_filename = name
        print("Binary filename set to: " + name)
        
    @pyqtSlot()
    def capture(self):
        while True:
            first_byte = server_sock.recv(1)
            if self.binary_mode:
                # In binary mode, we can't grab sentences and emit them as they come, since there aren't natural breaks
                # and in any case we don't want to spam the text record with random binary data.  The goal here is to
                # report how many bytes are expected, show the first few packets, and then confirm that it's done.  We
                # rely on the logger sending a length word as the first piece of data.
                transfer_size_buffer = first_byte + server_sock.recv(3)
                transfer_size = int.from_bytes(transfer_size_buffer, "little")
                buffer = "Binary transfer: " + transfer_size + " bytes to " + self.binary_filename
                self.messageReady.emit(buffer)
                f = open(self.binary_filename, 'wb')
                bytes_transferred = 0
                for b in range(transfer_size):
                    in_byte = server_sock.recv(1)
                    f.write(in_byte)
                    ++bytes_transferred
                f.close()
                buffer = "Transferred " + bytes_transferred + " bytes total"
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


class Window(QtWidgets.QMainWindow):
    binary_mode = pyqtSignal(boolean)
    binary_filename = pyqtSignal(str)
    
    def __init__(self):
        super(Window, self).__init__()
        uic.loadUi('gui.ui', self)
        self.lineEdit.returnPressed.connect(self.doCommand)

        # We start the capture thread that grabs output from the server, and buffers for display
        self.obj = CaptureReturn()
        self.binary_mode.connect(self.obj.set_binary_mode)
        self.binary_filename.connect(self.obj.set_binary_filename)
        self.binary_mode.emit(False)
        self.rx_thread = QThread()
        self.obj.messageReady.connect(self.outputMessageReady)
        self.obj.moveToThread(self.rx_thread)
        self.rx_thread.started.connect(self.obj.capture)
        self.rx_thread.start()

    def doCommand(self):
        cmd = self.lineEdit.text()
        self.lineEdit.setText("")
        self.updateRecord("\n$ " + cmd + "\n")
        if cmd.startswith("transfer"):
            self.binary_mode.emit(True)
            file_number = cmd.split(' ')[1]
            file_name = 'nmea2000.' + file_number
            self.binary_filename.emit(file_name)
        else:
            self.binary_mode.emit(False)
        server_sock.send(cmd.encode("utf8"))

    def outputMessageReady(self, message):
        self.updateRecord(message)

    def updateRecord(self, message):
        self.textBrowser.setText(self.textBrowser.toPlainText() + message)
        self.textBrowser.verticalScrollBar().setValue(self.textBrowser.verticalScrollBar().maximum())


server_address = "192.168.4.1"
server_port = 25443
print("Starting connection for logger on " + server_address + " port " + server_port)
server_sock = socket.create_connection((server_address, server_port))

app = QtWidgets.QApplication([])
win = Window()
win.show()
sys.exit(app.exec())
