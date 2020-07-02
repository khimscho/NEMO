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
                print("Exception: " + str(e))


class Window(QtWidgets.QMainWindow):
    
    def __init__(self):
        super(Window, self).__init__()
        uic.loadUi('gui.ui', self)
        self.lineEdit.returnPressed.connect(self.doCommand)

        # We start the capture thread that grabs output from the server, and buffers for display
        self.obj = CaptureReturn()
        self.rx_thread = QThread()
        self.obj.messageReady.connect(self.outputMessageReady)
        self.obj.moveToThread(self.rx_thread)
        self.rx_thread.started.connect(self.obj.capture)
        self.rx_thread.start()

    @pyqtSlot()
    def doCommand(self):
        global server_sock
        cmd = self.lineEdit.text()
        self.lineEdit.setText("")
        self.updateRecord("$ " + cmd + "\n")
        if cmd.startswith("binary"):
            mode = cmd.split(' ')[1]
            if mode == "on":
                set_transfer_mode(True)
            else:
                set_transfer_mode(False)
        else:
            if cmd.startswith("transfer"):
                set_transfer_mode(True)
                file_number = cmd.split(' ')[1]
                file_name = 'nmea2000.' + file_number
                set_transfer_filename(file_name)
            else:
                set_transfer_mode(False)
            server_sock.send(cmd.encode("utf8"))

    @pyqtSlot(str)
    def outputMessageReady(self, message):
        self.updateRecord(message)

    def updateRecord(self, message):
        self.textBrowser.setText(self.textBrowser.toPlainText() + message)
        self.textBrowser.verticalScrollBar().setValue(self.textBrowser.verticalScrollBar().maximum())

def set_transfer_mode(flag):
    global binary_mode
    binary_mode = flag
    
def set_transfer_filename(name):
    global binary_filename
    binary_filename = name
    
server_address = "192.168.4.1"
server_port = 25443
print("Starting connection for logger on " + str(server_address) + " port " + str(server_port))
server_sock = socket.create_connection((server_address, server_port))

binary_mode = False
binary_filename = ""

app = QtWidgets.QApplication([])
win = Window()
win.show()
sys.exit(app.exec())
