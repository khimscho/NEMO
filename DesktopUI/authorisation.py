# authorisation.py
#
# Generate a dialog box to manage authorisation specifications
#
# Copyright 2024 Center for Coastal and Ocean Mapping & NOAA-UNH Joint
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
from tkinter import filedialog
from tkinter import messagebox as mbox
from typing import Dict, AnyStr
import json
from command import LoggerInterface
import string
import random

class AuthDBox:
    hor_pad = 10
    ver_pad = 5
    
    def __init__(self, root, address: str, port: str, output_widget):
        self.root = tk.Toplevel(root)
        self.server_address = address
        self.server_port = port
        self.output_widget = output_widget

        self.main_frame = tk.Frame(self.root, padx=self.hor_pad, pady=self.ver_pad)
        self.main_frame.pack(fill='both')

        self.password_frame = tk.LabelFrame(self.main_frame, text='Password', padx=self.hor_pad, pady=self.ver_pad)
        self.password_frame.pack(fill='x')
        self.password_var = tk.StringVar()
        self.password_entry = tk.Entry(self.password_frame, textvariable=self.password_var)
        self.password_entry.grid(row=0, column=0, sticky='we')
        self.password_button = tk.Button(self.password_frame, text='Make Password', command=self.on_make_password)
        self.password_button.grid(row=0, column=1, sticky='e')

        self.cert_frame = tk.LabelFrame(self.main_frame, text='CA Certificate', padx=self.hor_pad, pady=self.ver_pad)
        self.cert_frame.pack()
        self.cert_scrollbar = tk.Scrollbar(self.cert_frame, orient='vertical')
        self.cert_scrollbar.pack(side=tk.RIGHT, fill='y')
        self.cert_text = tk.Text(self.cert_frame, yscrollcommand=self.cert_scrollbar.set)
        self.cert_scrollbar.config(command=self.cert_text.yview)
        self.cert_text.pack(fill='both')

        self.button_frame = tk.Frame(self.main_frame, padx=self.hor_pad, pady=self.ver_pad)
        self.button_frame.pack(fill='both')
        self.query_button = tk.Button(self.button_frame, text='Query Logger', command=self.on_query)
        self.query_button.grid(row=0, column=0)
        self.set_button = tk.Button(self.button_frame, text='Set Logger', command=self.on_set)
        self.set_button.grid(row=0, column=1)
        self.load_button = tk.Button(self.button_frame, text='Load Cert', command=self.on_loadcert)
        self.load_button.grid(row=0, column=2)
        self.dismiss_button = tk.Button(self.button_frame, text='Dismiss', command=self.on_dismiss)
        self.dismiss_button.grid(row=0, column=3, sticky='e')


    def on_make_password(self):
        choices = string.ascii_letters + string.digits
        password = []
        for i in range(32):
            password.append(random.choice(choices))
        self.password_var.set("".join(password))

    def on_query(self):
        interface = LoggerInterface(self.server_address, self.server_port)
        success, info = interface.execute_command('auth')
        if success:
            data = json.loads(info)
            if len(data['messages'][0]) > 0:
                self.password_var.set(data['messages'][0])
            if len(data['messages'][1]) > 0:
                self.cert_text.delete("1.0", tk.END)
                self.cert_text.insert("1.0", data['messages'][1])
        else:
            mbox.showerror(title='Aithorisation Setup', message='Logger authorisation query failed.')

    def on_set(self):
        password = self.password_var.get()
        cert = self.cert_text.get("1.0", tk.END)
        interface = LoggerInterface(self.server_address, self.server_port)
        success: bool = False
        info: str = ""
        if password is not None:
            success, info = interface.execute_command('auth token ' + password)
        if len(cert) > 1:
            success, info = interface.execute_command('auth cert ' + cert)
        if success:
            data = json.loads(info)
            if len(data['messages'][0]) > 0:
                self.password_var.set(data['messages'][0])
            if len(data['messages'][1]) > 0:
                self.cert_text.delete("1.0", tk.END)
                self.cert_text.insert("1.0", data['messages'][1])
        else:
            mbox.showerror(title='Authorisation Setup', message='Logger authorisation setup failed.')

    def on_loadcert(self):
        cert_filename = filedialog.askopenfilename(title='Select Certificate File', filetypes=[('Certificate Files', '*.crt')])
        if cert_filename:
            with open(cert_filename, 'r') as f:
                data = f.read()
            self.cert_text.delete("1.0", tk.END)
            self.cert_text.insert("1.0", data)

    def on_dismiss(self):
        self.root.destroy()
