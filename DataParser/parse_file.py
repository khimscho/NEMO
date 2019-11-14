import sys
import LoggerFile

file = open(sys.argv[1], 'rb')

packet_count = 0
source = LoggerFile.PacketFactory(file)
while source.has_more():
    pkt = source.next_packet()
    if pkt is not None:
        print(pkt)
        packet_count += 1

print("Found " + str(packet_count) + " packets total")
