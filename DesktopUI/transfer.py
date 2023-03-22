# transfer.py
#
# Generate a dialog box to transfer files from the logger and save them on disc
#
# Copyright 2023 Center for Coastal and Ocean Mapping & NOAA-UNH Joint
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

import tkinter as tk
from tkinter import filedialog, messagebox
from command import LoggerInterface
import hashlib

class TransferDBox:
    hor_pad = 10
    ver_pad = 5

    def __init__(self, root, address: str, port: str, output_widget):
        self.root = tk.Toplevel(root)
        self.server_address = address
        self.server_port = port
        self.output_widget = output_widget

        self.main_frame = tk.Frame(self.root, padx=self.hor_pad, pady=self.ver_pad)
        self.main_frame.pack(fill='both')

        self.filenum_var = tk.IntVar()
        self.filename_var = tk.StringVar()

        self.filenum_label = tk.Label(self.main_frame, text='File Number:')
        self.filenum_entry = tk.Entry(self.main_frame, textvariable=self.filenum_var)
        self.filenum_label.grid(row=0, column=0, sticky='e')
        self.filenum_entry.grid(row=0, column=1, sticky='w')
        self.filename_label = tk.Label(self.main_frame, text='Output File:')
        self.filename_entry = tk.Entry(self.main_frame, textvariable=self.filename_var)
        self.filename_button = tk.Button(self.main_frame, text='...', command=self.on_filename)
        self.filename_label.grid(row=1, column=0, sticky='e')
        self.filename_entry.grid(row=1, column=1, sticky='w')
        self.filename_button.grid(row=1, column=2, sticky='w')

        self.button_frame = tk.Frame(self.main_frame)
        self.button_frame.grid(row=2, column=0, columnspan=3)
        self.transfer_button = tk.Button(self.button_frame, text='Transfer', command=self.on_transfer)
        self.dismiss_button = tk.Button(self.button_frame, text='Dismiss', command=self.on_dismiss)
        self.transfer_button.grid(row=0, column=0)
        self.dismiss_button.grid(row=0, column=1)

    def on_filename(self):
        filenum = self.filenum_var.get()
        filename = filedialog.asksaveasfilename(title='Select Output File', initialfile=f'wibl-raw.{filenum}.wibl', confirmoverwrite=True, defaultextension='wibl')
        if filename:
            self.filename_var.set(filename)
            self.filename_entry.xview_moveto(1.0)

    def on_transfer(self):
        filename = self.filename_var.get()
        filenum = self.filenum_var.get()
        interface = LoggerInterface(self.server_address, self.server_port)
        status, data, digest = interface.get_file(filenum)
        if status:
            with open(filename, 'wb') as f:
                f.write(data)
            local_hash = hashlib.md5()
            local_hash.update(data)
            local_digest = local_hash.hexdigest()
            self.record(f'info: wrote file {filenum} to {filename}.\n')
            if local_digest.upper() != digest.upper():
                self.record('error: local hash does not match the transferred hash value.\n')
                dbx = messagebox.showwarning(title='Hash Mismatch', message='File hash from the logger does not match the local hash.  Check validity!')
        else:
            self.record(f'error: failed to write file {filenum} to {filename}.\n')
            self.record(data + '\n') # detailed error message from the loger interface
    
    def record(self, message: str) -> None:
        self.output_widget.insert(tk.END, message)
        self.output_widget.yview_moveto(1.0)

    def on_dismiss(self):
        self.root.destroy()
