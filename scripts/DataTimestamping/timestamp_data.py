# \file timestamp_data.py
# \brief Read a binary data file from the SB2030 data logger, and generate timestamped data
#
# The SB2030 data logger reports, for each packet received, the elapsed time with respect
# to the logger's time ticker, and the data.  It also estimates the real time for the packet's
# reception time using whatever time information it has available, in real-time, using a forward
# extrapolation of the last time reference.  Of course, this isn't great, because it has to be
# causal.  This code is designed to get around that by reading all of the data to find the best
# time reference information, and then going back through again to provide timestamps for the
# packets based on all of the time reference data.
#    The code is pretty simple, just doing linear interpolation between timestamps of the reference
# time in order to generate the timestamps.  Reference time is, by preference NMEA2000 SystemTime,
# then the timestamps from NMEA2000 GNSS packets, and then finally from NMEA0183 GPGGA packets if
# there's nothing else available.
#
# Copyright 2020 Center for Coastal and Ocean Mapping & NOAA-UNH Joint
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
import argparse as arg
from datetime import datetime

import wibl.core.timestamping as ts

def main():
    parser = arg.ArgumentParser(description = 'Convert WIBL logger data to timestamped ASCII')
    parser.add_argument('-b', '--bitsize', help = 'Set bit-length of elapsed times')
    parser.add_argument('-i', '--input', help = 'WIBL format input file')
    parser.add_argument('-o', '--output', help = 'ASCII format output file')
    parser.add_argument('-q', '--heading', help = 'ASCII format output for true heading')
    parser.add_argument('-t', '--temp', help = 'ASCII format output for water temperature')
    parser.add_argument('-w', '--wind', help = 'ASCII format output for wind speed/direction')
    parser.add_argument('input', help = 'WIBL format input file')
    parser.add_argument('output', help = 'ASCII format output file')

    optargs = parser.parse_args()
    
    if optargs.bitsize:
        elapsed_time_quantum = 1 << int(optargs.bitsize)
    else:
        elapsed_time_quantum = 1 << 32
        
    if optargs.input:
        in_filename = optargs.input
        
    if optargs.output:
        out_filename = optargs.output
    
    try:
        tsdata = ts.time_interpolation(in_filename, elapsed_time_quantum)
        
    except ts.NoTimeSource:
        print('Error: failed to find a time source to timestamp file.')
        sys.exit(1)
        
    except ts.NoData:
        print('Warning: no data to interpolate.')
        sys.exit(1)
    
    print('Converting Data For:');
    print('Logger Name:\t', tsdata['loggername'])
    print('Platform Name:\t', tsdata['platform'])
    
    with open(out_filename, 'w') as f:
        f.write('Time,Epoch,Longitude,Latitude,Depth\n')
        for i in range(len(tsdata['depth']['t'])):
            dt = datetime.fromtimestamp(tsdata['depth']['t'][i])
            f.write('%s,%.3f,%.8f,%.8f,%.2f\n' % (dt, tsdata['depth']['t'][i], tsdata['depth']['lon'][i], tsdata['depth']['lat'][i], tsdata['depth']['z'][i]))
    
    if optargs.heading:
        with open(optargs.heading, 'w') as f:
            f.write('Time,Epoch,Longitude,Latitude,Heading\n')
            for i in range(len(tsdata['heading']['t'])):
                dt = datetime.fromtimestamp(tsdata['heading']['t'][i])
                f.write('%s,%.3f,%.8f,%.8f,%.1f\n' % (dt, tsdata['heading']['t'][i], tsdata['heading']['lon'][i], tsdata['heading']['lat'][i], tsdata['heading']['heading'][i]))
    
    if optargs.temp:
        with open(optargs.temp, 'w') as f:
            f.write('Time,Epoch,Longitude,Latitude,Temperature\n')
            for i in range(len(tsdata['watertemp']['t'])):
                dt = datetime.fromtimestamp(tsdata['watertemp']['t'][i])
                f.write('%s,%.3f,%.8f,%.8f,%.1f\n' % (dt, tsdata['watertemp']['t'][i], tsdata['watertemp']['lon'][i], tsdata['watertemp']['lat'][i], tsdata['watertemp']['temperature'][i]))
                
    if optargs.wind:
        with open(optargs.wind, 'w') as f:
            f.write('Time,Epoch,Longitude,Latitude,Direction,Speed\n')
            for i in range(len(tsdata['wind']['t'])):
                dt = datetime.fromtimestamp(tsdata['wind']['t'][i])
                f.write('%s,%.3f,%.8f,%.8f,%.2f,%.2f\n' % (dt, tsdata['wind']['t'][i], tsdata['wind']['lon'][i], tsdata['wind']['lat'][i], tsdata['wind']['direction'][i], tsdata['wind']['speed'][i]))

if __name__ == "__main__":
    main()
