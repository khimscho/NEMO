# \file dedup_depth.py
# \brief Remove duplicated depths from a WIBL file in CSV format
#
# This code attempts to remove from a WIBL file in CSV format any lines with
# an exactly replicated depth.  This can be a problem in systems with repeaters,
# for example.
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

import sys
import pandas
import argparse as arg

def main():
    parser = arg.ArgumentParser(description = 'Plot WIBL logger data in CSV format')
    parser.add_argument('input', help = 'CSV format input file')
    parser.add_argument('output', help = 'CSV format output file')
    
    optargs = parser.parse_args()
    
    if optargs.input:
        data = pandas.read_csv(optargs.input)
    else:
        print('Error: must have at least an input file.');
        sys.exit(1)
        
    if optargs.output:
        out_file = optargs.output
    else:
        print('Error: must have an output file.');
        sys.exit(1)
    
    with open(out_file, 'w') as f:
        f.write('Time,Epoch,Longitude,Latitude,Depth\n')
        current_depth = 0
        for n in range(0,len(data)):
            if data['Depth'][n] != current_depth:
                f.write('%s,%.3f,%.8f,%.8f,%.2f\n' % (data['Time'][n], data['Epoch'][n], data['Longitude'][n], data['Latitude'][n], data['Depth'][n]))
                current_depth = data['Depth'][n]

        
if __name__ == "__main__":
    main()
