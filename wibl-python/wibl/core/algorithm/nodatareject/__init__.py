##\module nodatareject
# \brief Algorithm for rejecting no-data values (for position, depth, or time) in the provided data
#
# Currently no-data filtering is only performed on NMEA 2000 packets.
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
from typing import List, Dict

from wibl import __version__ as wiblversion
from wibl.core.logger_file import DataPacket
from wibl.core.algorithm import AlgorithmPhase, WiblAlgorithm
from core import Lineage

__version__ = '1.0.0'

ALG_NAME = 'nodatareject'



def reject_nodata(data: List[DataPacket], params: str,  lineage: Lineage, verbose: bool) -> List[DataPacket]:
    for i, pkt in enumerate(data):
        if verbose:
            print(f"pkt {i+1} is of type {type(pkt)}")
    return data


class NoDataReject(WiblAlgorithm):
    name: str = ALG_NAME
    phases: AlgorithmPhase = AlgorithmPhase.ON_LOAD

    @classmethod
    def run_on_load(cls,
                    data: List[DataPacket],
                    params: str,
                    lineage: Lineage,
                    verbose: bool) -> List[DataPacket]:
        return reject_nodata(data, params, lineage, verbose)
