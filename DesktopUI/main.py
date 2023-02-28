# main.py
#
# Provide the primary control for a Tkinter interface to the WIBL embedded web-server.
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
from tkinter import filedialog
from command import LoggerInterface
import json

__interface_version__ = "1.0.0"
__default_server_ip__ = '192.168.4.1'
__default_server_port__ = '80'

class MainWindow:
    hor_pad = 10
    ver_pad = 5

    def __init__(self, root: tk.Tk, server_ip: str = __default_server_ip__, server_port: str = __default_server_port__) -> None:
        self.main_frame = tk.Frame(root, padx=self.hor_pad, pady=self.ver_pad)
        self.main_frame.pack(fill='both')

        # Set up server address and port
        self.server_address_var = tk.StringVar()
        self.server_address_var.set(server_ip)

        self.server_port_var = tk.StringVar()
        self.server_port_var.set(server_port)

        self.server_frame = tk.LabelFrame(self.main_frame, text='Server', padx=self.hor_pad, pady=self.ver_pad)
        self.address_label = tk.Label(self.server_frame, text='IP Address', anchor='e')
        self.address_entry = tk.Entry(self.server_frame, textvariable=self.server_address_var)
        self.address_label.grid(row=0,column=0)
        self.address_entry.grid(row=0,column=1)

        self.port_label = tk.Label(self.server_frame, text='Port', anchor='e')
        self.server_port_entry = tk.Entry(self.server_frame, textvariable=self.server_port_var)
        self.port_label.grid(row=1,column=0)
        self.server_port_entry.grid(row=1,column=1)
        
        self.server_frame.pack(fill='x')
        
        # Set up command space
        self.command_var = tk.StringVar()

        self.command_frame = tk.LabelFrame(self.main_frame, text='Command', padx=self.hor_pad, pady=self.ver_pad)
        self.command_entry = tk.Entry(self.command_frame, textvariable=self.command_var)
        self.command_entry.bind('<Return>', self.on_command)
        self.command_entry.pack(fill='x')

        self.command_frame.pack(fill='x')

        # Set up output frame and text space
        self.output_frame = tk.LabelFrame(self.main_frame, text='Output', padx=self.hor_pad, pady=self.ver_pad)
        self.output_scrollbar = tk.Scrollbar(self.output_frame, orient='vertical')
        self.output_scrollbar.pack(side=tk.RIGHT, fill='y')
        self.output_text = tk.Text(self.output_frame, yscrollcommand=self.output_scrollbar.set)
        self.output_scrollbar.config(command=self.output_text.yview)
        self.output_text.pack(fill='both')

        self.output_frame.pack(fill='both')

        # Set up buttons for direct actions
        self.button_frame = tk.LabelFrame(self.main_frame, text='Actions', padx=self.hor_pad, pady=self.ver_pad)
        self.config_button = tk.Button(self.button_frame, text='Configure')
        self.status_button = tk.Button(self.button_frame, text='Status')
        self.metadata_button = tk.Button(self.button_frame, text='Metadata', command=self.on_metadata)
        self.config_button.grid(row=0,column=0)
        self.status_button.grid(row=0,column=1)
        self.metadata_button.grid(row=0,column=2)

        self.button_frame.pack(fill='x')

    def run_command(self, command: str) -> str:
        interface = LoggerInterface(self.server_address_var.get(), self.server_port_var.get())
        result = interface.execute_command(command)
        return result
    
    def update_output(self, output: str) -> None:
        self.output_text.insert(tk.END, output)
        self.output_text.yview_moveto(1.0)
    
    def on_command(self, entry):
        command = self.command_entry.get()
        self.output_text.insert(tk.END, '>>> ' + command + '\n')
        self.command_entry.delete(0, tk.END)

        result = self.run_command(command)
        self.update_output(result)

    def on_metadata(self):
        json_filename = filedialog.askopenfilename(title='Select JSON Metadata File', filetypes=[('JSON Files', '*.json')])
        with open(json_filename, 'r') as f:
            data = json.load(f)
        command: str = 'metadata ' + json.dumps(data)
        result = self.run_command(command)
        self.update_output(result)

def main():
    root = tk.Tk()
    root.title("WIBL Data Management Interface" + __interface_version__)
    main_window = MainWindow(root)
    root.mainloop()

if __name__ == "__main__":
    main()
