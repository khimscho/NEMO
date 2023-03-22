# command.py
#
# Provide the interface to the webserver
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

import requests
from typing import Tuple

class LoggerInterface:
    def __init__(self, address: str, port: str) -> None:
        self.server_url = f'http://{address}:{port}/command'

    def execute_command(self, command: str) -> Tuple[bool, str]:
        try:
            result = requests.post(self.server_url, data = {'command': command}, timeout=3)
            if result.status_code == 200:
                rtn = (True, result.text)
            else:
                rtn = (False, f'Error: logger returned status {result.status_code} and information {result.text}')
        except requests.exceptions.ConnectTimeout:
            rtn = (False, f'Error: connection timed out - is the logger on?\n')
        return rtn

    def get_file(self, number: int) -> Tuple[bool, bytes, str]:
        try:
            result = requests.post(self.server_url, data = {'command': f'transfer {number}'}, timeout=3)
            if result.status_code == 200:
                digest_alg, digest_val = result.headers['Digest'].split('=')
                rtn = (True, result.content, digest_val)
            else:
                rtn = (False, f'Error: logger returned status {result.status_code} and information {result.text}', None)
        except requests.exceptions.ConnectTimeout:
            rtn = (False, f'Error: connection timed out - is the logger on?', None)
        return rtn

def run_command(address: str, port: str, command: str) -> str:
    interface = LoggerInterface(address, port)
    result = interface.execute_command(command)
    return result