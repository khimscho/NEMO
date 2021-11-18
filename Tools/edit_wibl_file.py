##\file edit_wibl_file.py
# \brief Limited editing/modification of WIBL raw binary files
#
# Although the goal is to have all of the data points required for the WIBL binary
# file embedded into the logger before data capture starts, it's occasionally
# necessary to edit one or more of the metadata elements of the files to correct issues
# that might not have been obvious at startup.  This code allows for some editing
# in batch (i.e., non-interactive) mode.
#
# Copyright 2021 Center for Coastal and Ocean Mapping & NOAA-UNH Joint
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

import LoggerFile as lf
import argparse as arg
from sys import exit
import json

def main():
    parser = arg.ArgumentParser(description = 'Edit WIBL logger files (in a limited capacity)')
    parser.add_argument('-n', '--name', type=str, help = 'Set the logger name (string)')
    parser.add_argument('-u', '--uniq', type=str, help = 'Set the unique ID for the logger (string)')
    parser.add_argument('-m', '--meta', type=str, help = 'Specify a JSON file for additional metadata elements (filename)')
    parser.add_argument('-a', '--algo', type=str, action='append', help = 'Add a processing algorithm request (name: str,params: str)')
    parser.add_argument('input', type=str, help = 'WIBL format input file')
    parser.add_argument('output', type=str, help = 'WIBL format output file')

    optargs = parser.parse_args()

    if not optargs.input:
        print('Error: must have an input file!')
        exit(1)
    
    if not optargs.output:
        print('Error: must have an output file!')
        exit(1)

    logger_name = None
    unique_id = None
    metadata = None
    algorithms = []
    if optargs.name:
        logger_name = optargs.name
    if optargs.uniq:
        unique_id = optargs.uniq
    if optargs.meta:
        with open(optargs.meta) as f:
            json_meta = json.load(f)
        metadata = json_meta.dumps()
    if optargs.algo:
        for alg in optargs.algo:
            name,params = alg.split(',')
            algorithms.append({ 'name': name, 'params': params})

    # Next step is to loop through the data file, copying packets by default
    # unless there's something that we have to add.  Note that we edit the
    # Metadata packet to change the logger name and/or unique ID, if it exists
    # (it should on most systems), but add it later if there isn't one; with the
    # JSON metadata, the same thing is true: if the packet exists, we replace it
    # at the same location in the file, but otherwise append it at the end.
    op = open(optargs.output, 'wb')
    metadata_out = False
    json_metadata_out = False
    with open(optargs.input, 'rb') as ip:
        source = lf.PacketFactory(ip)
        while source.has_more():
            packet = source.next_packet()
            if packet:
                if isinstance(packet, lf.Metadata):
                    if logger_name or unique_id:
                        out_name = packet.logger_name
                        out_id = packet.ship_name
                        if logger_name:
                            out_name = logger_name
                        if unique_id:
                            out_id = unique_id
                        packet = lf.Metadata(logger = out_name, uniqid = out_id)
                        metadata_out = True
                elif isinstance(packet, lf.JSONMetadata):
                    if metadata:
                        packet = lf.JSONMetadata(meta = metadata)
                        json_metadata_out = True
                packet.serialise(op)
        # At the end of the file, if we haven't yet sent out any of the edited packets,
        # we just append
        if not metadata_out:
            if logger_name or unique_id:
                out_name = 'Unknown'
                if logger_name:
                    out_name = logger_name
                out_id = 'Unknown'
                if unique_id:
                    out_id = unique_id
                packet = lf.Metadta(logger = out_name, uniqid = out_id)
                packet.serialise(op)
        if not json_metadata_out and metadata:
            packet = lf.JSONMetadata(meta = metadata)
            packet.serialise(op)
        if algorithms:
            for alg in algorithms:
                packet = lf.AlgorithmRequest(name = alg['name'], params = alg['params'])
                packet.serialise(op)
    
    op.flush()
    op.close()

if __name__== "__main__":
    main()
