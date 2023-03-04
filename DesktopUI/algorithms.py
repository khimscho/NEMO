# algorithms.py
#
# Generate a dialog box to manage algorithm specifications
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

from typing import Dict, Any
import tkinter as tk
from tkintertable import TableCanvas
from tkinter import filedialog
from command import LoggerInterface
import json

class AlgoDBox:
    def __init__(self, root, address: str, port: str, output_widget: tk.Text):
        self.root = tk.Toplevel(root)
        self.server_address = address
        self.server_port = port
        self.output_widget = output_widget

        self.main_frame = tk.Frame(self.root)
        self.main_frame.pack(fill='both')

        self.table_frame = tk.Frame(self.main_frame)
        self.table_frame.pack(fill='both')

        data = {'rec1': {'name': '', 'parameters': ''}}
        self.algo_table = TableCanvas(self.table_frame, data=data, thefont=('Arial', 14), rowheight=40)
        self.algo_table.model.columnlabels['name'] = 'Algorithm Name'
        self.algo_table.model.columnlabels['parameters'] = 'Algorithm Parameters'
        self.algo_table.show()

        self.button_frame = tk.Frame(self.main_frame)
        self.button_frame.pack(fill='x')
        
        self.getconfig_button = tk.Button(self.button_frame, text='Query Logger', command=self.on_query)
        self.setconfig_button = tk.Button(self.button_frame, text='Set Logger', command=self.on_set)
        self.loadconfig_button = tk.Button(self.button_frame, text='Load Config', command=self.on_load)
        self.saveconfig_button = tk.Button(self.button_frame, text='Save Config', command=self.on_save)
        self.dismiss_button = tk.Button(self.button_frame, text='Dismiss', command=self.on_dismiss)
        self.getconfig_button.grid(row=0,column=0)
        self.setconfig_button.grid(row=0,column=1)
        self.loadconfig_button.grid(row=0,column=2)
        self.saveconfig_button.grid(row=0,column=3)
        self.dismiss_button.grid(row=0,column=4)

    def dict_to_table(self, input: Dict[str,Any]) -> Dict[str,Any]:
        data = {}
        if 'algorithm' in input:
            for alg in range(len(input['algorithm'])):
                data[f'rec{alg+1}'] = input['algorithm'][alg]
        else:
            data['rec1'] = {'name': '', 'parameters': ''}
        return data

    def table_to_dict(self, input: Dict[str,Any]) -> Dict[str,Any]:
        data = {'algorithm': []}
        for alg in input.values():
            data['algorithm'].append({'name': alg[0], 'parameters': alg[1]})
        return data

    def on_query(self):
        interface = LoggerInterface(self.server_address, self.server_port)
        success, info = interface.execute_command('algorithm')
        if success:
            data = self.dict_to_table(json.loads(info))
            self.algo_table.model.importDict(data)
            self.algo_table.redraw()
        else:
            self.record(f'Error: failed to get algorithms from logger\n{info}\n')

    def on_set(self):
        algorithms = self.table_to_dict(self.algo_table.model.getAllCells())
        interface = LoggerInterface(self.server_address, self.server_port)
        success, info = interface.execute_command('algorithm none')
        if not success:
            self.record(f'Error: failed to clear algorithms on logger\n{info}\n')
            return
        for alg in algorithms['algorithm']:
            success, info = interface.execute_command(f'algorithm {alg["name"]} {alg["parameters"]}')
            if not success:
                self.record(f'Error: failed to set algorithm {alg["name"]} on logger\n{info}\n')
                return
        self.record(f'Info: success in reset algorithms on logger.\n')

    def on_load(self):
        filename = filedialog.askopenfilename(title='Select Algorithm JSON file')
        if filename:
            with open(filename, 'r') as f:
                algorithms = json.load(f)
            if 'algorithm' not in algorithms:
                self.record('Error: failed to find algorithm element in JSON specification - is that the right file?')
            else:
                table_data = self.dict_to_table(algorithms)
                self.algo_table.model.importDict(table_data)
                self.algo_table.redraw()

    def on_save(self):
        filename = filedialog.asksaveasfilename(title='Select Output JSON File', confirmoverwrite=True, initialfile='logger-algorithms.json', defaultextension='json')
        if filename:
            rendered = self.table_to_dict(self.algo_table.model.getAllCells())
            with open(filename, 'w') as f:
                json.dump(rendered, f, indent=4)

    def on_dismiss(self):
        self.root.destroy()
    
    def record(self, message: str) -> None:
        self.output_widget.insert(tk.END, message)
        self.output_widget.yview_moveto(1.0)