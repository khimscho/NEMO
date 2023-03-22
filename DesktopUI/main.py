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
from configure import ConfigDBox
from transfer import TransferDBox
from algorithms import AlgoDBox
from filters import NMEA0183FilterDBox
import json
from typing import Tuple
from io import StringIO

__interface_version__ = "1.0.0"
__default_server_ip__ = '192.168.4.1'
__default_server_port__ = '80'

class MainWindow:
    hor_pad = 10
    ver_pad = 5

    def __init__(self, root: tk.Tk, server_ip: str = __default_server_ip__, server_port: str = __default_server_port__) -> None:
        self.root = root

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
        self.address_label.grid(row=0, column=0)
        self.address_entry.grid(row=0, column=1)

        self.port_label = tk.Label(self.server_frame, text='Port', anchor='e')
        self.server_port_entry = tk.Entry(self.server_frame, textvariable=self.server_port_var)
        self.port_label.grid(row=1, column=0)
        self.server_port_entry.grid(row=1, column=1)
        
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
        self.setup_button = tk.Button(self.button_frame, text='Setup', command=self.on_setup)
        self.status_button = tk.Button(self.button_frame, text='Status', command=self.on_status)
        self.metadata_button = tk.Button(self.button_frame, text='Metadata', command=self.on_metadata)
        self.algorithm_button = tk.Button(self.button_frame, text='Algorithms', command=self.on_algorithms)
        self.nmea0183_button = tk.Button(self.button_frame, text='NMEA0183 Filter', command=self.on_filter)
        self.transfer_button = tk.Button(self.button_frame, text='Transfer Data', command=self.on_transfer)
        self.restart_button = tk.Button(self.button_frame, text='Restart', command=self.on_restart)
        self.setup_button.grid(row=0, column=0)
        self.status_button.grid(row=0, column=1)
        self.metadata_button.grid(row=0, column=2)
        self.algorithm_button.grid(row=0, column=3)
        self.nmea0183_button.grid(row=0, column=4)
        self.transfer_button.grid(row=0, column=5)
        self.restart_button.grid(row=0, column=6)

        self.button_frame.pack(fill='x')

    def run_command(self, command: str) -> Tuple[bool, str]:
        interface = LoggerInterface(self.server_address_var.get(), self.server_port_var.get())
        success, info = interface.execute_command(command)
        return success, info
    
    def update_output(self, output: str) -> None:
        self.output_text.insert(tk.END, output)
        self.output_text.yview_moveto(1.0)
    
    def on_command(self, entry):
        command = self.command_entry.get()
        if command:
            self.output_text.insert(tk.END, '>>> ' + command + '\n')
            self.command_entry.delete(0, tk.END)
            success, info = self.run_command(command)
            self.update_output(info + '\n')
    
    def on_setup(self):
        config_dbox = ConfigDBox(self.root, self.server_address_var.get(), self.server_port_var.get(), self.output_text)
        self.root.wait_window(config_dbox.root)
    
    def on_status(self):
        success, info = self.run_command('status')
        if success:
            status = json.loads(info)
            summary = StringIO()
            summary.write('Status Summary: \n  Versions:\n')
            summary.write(f'    CommandProc: {status["version"]["commandproc"]}\n')
            summary.write(f'    NMEA0183:    {status["version"]["nmea0183"]}\n')
            summary.write(f'    NMEA2000:    {status["version"]["nmea2000"]}\n')
            summary.write(f'    IMU:         {status["version"]["imu"]}\n')
            summary.write(f'    Serialiser:  {status["version"]["serialiser"]}\n')
            up_time: float = status["elapsed"]/1000
            up_time_rep: str = ''
            if up_time > 24*60*60:
                up_time_days: int = int(up_time / (24*60*60))
                up_time_rep += f'{up_time_days} days'
                up_time -= up_time_days * 24*60*60
            if up_time > 60*60:
                up_time_hours: int = int(up_time/(60*60))
                up_time_rep += f' {up_time_hours} hours'
                up_time -= up_time_hours * 60*60
            if up_time > 60:
                up_time_mins: int = int(up_time / 60)
                up_time_rep += f' {up_time_mins} mins'
                up_time -= up_time_mins * 60
            up_time_rep += f' {up_time:.3f} s.'
            summary.write(f'  Elapsed Time: {up_time_rep}\n')
            summary.write(f'  Webserver Status: {status["webserver"]["current"]}\n')
            summary.write(f'  Files On Logger: {status["files"]["count"]}\n')
            file_size_total: int = 0
            for file in range(status["files"]["count"]):
                file_size_total += status["files"]["detail"][file]["len"]
            if file_size_total > 1024*1024*1024:
                file_size_total = file_size_total / (1024*1024*1024)
                size_units = 'GB'
            elif file_size_total > 1024*1024:
                file_size_total = file_size_total / (1024*1024)
                size_units = 'MB'
            elif file_size_total > 1024:
                file_size_total = file_size_total / 1024
                size_units = 'kB'
            else:
                size_units = 'B'
            summary.write(f'  Total File Size: {file_size_total:.3f} {size_units}\n')
            summary.seek(0)
            self.update_output(summary.read())
        else:
            self.update_output(info + '\n')

    def on_metadata(self):
        json_filename = filedialog.askopenfilename(title='Select JSON Metadata File', filetypes=[('JSON Files', '*.json')])
        if json_filename:
            with open(json_filename, 'r') as f:
                data = json.load(f)
            command: str = 'metadata ' + json.dumps(data)
            status, info = self.run_command(command)
            self.update_output(info)

    def on_algorithms(self):
        algo_dbox = AlgoDBox(self.root, self.server_address_var.get(), self.server_port_var.get(), self.output_text)
        self.root.wait_window(algo_dbox.root)

    def on_filter(self):
        filter_dbox = NMEA0183FilterDBox(self.root, self.server_address_var.get(), self.server_port_var.get(), self.output_text)
        self.root.wait_window(filter_dbox.root)

    def on_restart(self):
        status, info = self.run_command('restart')
        self.update_output(info + '\n')

    def on_transfer(self):
        transfer_dbox = TransferDBox(self.root, self.server_address_var.get(), self.server_port_var.get(), self.output_text)
        self.root.wait_window(transfer_dbox.root)

def main():
    root = tk.Tk()
    root.title("WIBL Data Management Interface " + __interface_version__)
    main_window = MainWindow(root)
    root.mainloop()

if __name__ == "__main__":
    main()
