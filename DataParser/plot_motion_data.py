## @file plot_motion_data.py
# @brief Driver to read and plot motion data from the logger file.
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
import LoggerFile
import matplotlib.pyplot as plt
import numpy as np

file = open(sys.argv[1], 'rb')

times = []
acc = []
gyro = []
temp = []

packet_count = 0
motion_count = 0

source = LoggerFile.PacketFactory(file)

while source.has_more():
    pkt = source.next_packet()
    if pkt is not None:
        packet_count += 1
        if isinstance(pkt, LoggerFile.Motion):
            motion_count += 1
            times.append(pkt.elapsed / 1000)
            acc.append((pkt.accel[0], pkt.accel[1], pkt.accel[2], np.sqrt(pkt.accel[0]**2 + pkt.accel[1]**2 + pkt.accel[2]**2)))
            gyro.append(pkt.gyro)
            temp.append(pkt.temp)

print("Found " + str(packet_count) + " packets total")
print("Found " + str(motion_count) + " motion packets")

plt.figure(figsize=(14,10))

plt.subplot(3,1,1)
plt.plot(times, temp)
plt.grid()
plt.xlabel('Elapsed Time (s)')
plt.ylabel('Sensor Temperature (oC)')

plt.subplot(3,1,2)
plt.plot(times, acc)
plt.grid()
plt.xlabel('Elapsed Time (s)')
plt.ylabel('Accelerations (G)')
plt.legend(('X-Axis', 'Y-Axis', 'Z-Axis', 'Magnitude'))

plt.subplot(3,1,3)
plt.plot(times, gyro)
plt.grid()
plt.xlabel('Elapsed Time (s)')
plt.ylabel('Rotation Rate (deg/s)')
plt.legend(('X-Axis', 'Y-Axis', 'Z-Axis'))

plt.show()
