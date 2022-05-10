from typing import BinaryIO, NoReturn

from wibl.core.logger_file import PacketTypes


class Writer:
    def __init__(self, filename: str):
        # Keep the filename for logging purposes
        self.filename: str = filename
        # Current output log file on the SD card
        self._m_output_log: BinaryIO = None
        # Object to handle serialisation of data (not sure we need this yet)
        # self._m_serializer: ? = None

    def record(pkt_id: PacketTypes, data):
        """
        Write a packet into the current log file
        :param data: ?! NOT SURE WHAT TYPE THIS NEEDS TO BE YET !?
        :return:
        """
        pass
