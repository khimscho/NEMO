from PyQt5 import QtWidgets, uic
from PyQt5.QtWidgets import QMessageBox, QDialog, QFileDialog
from PyQt5.QtCore import QThread, QObject, pyqtSignal, pyqtSlot
import sys
import os
import socket
import threading
import time


class CaptureReturn(QObject):
    messageReady = pyqtSignal(list)
    def __init__(self):
        QObject.__init__(self)

    @pyqtSlot()
    def capture(self):
        while True:
            buffer = ""
            sentence_complete = False
            while not sentence_complete:
                byte = server_sock.recv(1)
                ch = byte.decode("utf8")
                if ch != '\r':
                    buffer += ch
                if ch == '\n':
                    sentence_complete = True
            self.messageReady.emit(buffer)


class Window(QtWidgets.QMainWindow):
    def __init__(self):
        super(Window, self).__init__()
        uic.loadUi('gui.ui', self)
        self.lineEdit.returnPressed.connect(self.doCommand)

        # We start the capture thread that grabs output from the server, and buffers for display
        self.obj = CaptureReturn()
        self.thread = QThread()
        self.obj.messageReady.connect(self.outputMessageReady)
        self.obj.moveToThread(self.thread)
        self.thread.started.connect(self.obj.capture)
        self.thread.start()

    def doCommand(self):
        cmd = self.lineEdit.text()
        self.lineEdit.setText("")
        self.updateRecord("\n$ " + cmd)
        server_sock.send(cmd)

    def outputMessageReady(self, message):
        self.updateRecord(message)

    def updateRecord(self, message):
        self.textBrowser.setText(self.textBrowser.toPlainText() + message)
        self.textBrowser.verticalScrollBar().setValue(self.textBrowser.verticalScrollBar().maximum())


server_address = "192.168.4.1"
server_port = 25443
server_sock = socket.create_connection((server_address, server_port))

app = QtWidgets.QApplication([])
win = Window()
win.show()
sys.exit(app.exec())
