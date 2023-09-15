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
from typing import List
from datetime import datetime, timezone

import wibl.core.logger_file as LoggerFile
from wibl.core.algorithm import SOURCE, AlgorithmPhase, WiblAlgorithm
from core import Lineage

__version__ = '1.0.0'

ALG_NAME = 'nodatareject'
NA_DATA_DOUBLE: float = -1e9
EPOCH_START = datetime(1970, 1, 1).replace(tzinfo=timezone.utc)
SECONDS_PER_DAY = 86_400


def get_days_since_epoch() -> int:
    return (datetime.utcnow().replace(tzinfo=timezone.utc) - EPOCH_START).days


def reject_nodata(data: List[LoggerFile.DataPacket],
                  params: str,
                  lineage: Lineage,
                  verbose: bool) -> List[LoggerFile.DataPacket]:
    days_since_epoch: int = get_days_since_epoch()
    num_packets: int = 0
    num_packets_gnss: int = 0
    num_filtered_gnss: int = 0
    num_packets_depth: int = 0
    num_filtered_depth: int = 0
    num_packets_systime: int = 0
    num_filtered_systime: int = 0
    for i, pkt in enumerate(data):
        num_packets += 1
        if isinstance(pkt, LoggerFile.GNSS):
            num_packets_gnss += 1
            if pkt.longitude == NA_DATA_DOUBLE or pkt.latitude == NA_DATA_DOUBLE:
                if verbose:
                    print(f"Filtering out packet {str(pkt)}")
                num_filtered_gnss += 1
                del data[i]
                continue
        if isinstance(pkt, LoggerFile.Depth):
            num_packets_depth += 1
            if pkt.depth == NA_DATA_DOUBLE:
                if verbose:
                    print(f"Filtering out packet {str(pkt)}")
                num_filtered_depth += 1
                del data[i]
                continue
        if isinstance(pkt, LoggerFile.SystemTime):
            num_packets_systime += 1
            if pkt.date > days_since_epoch or pkt.elapsed > SECONDS_PER_DAY:
                if verbose:
                    print(f"Filtering out packet {str(pkt)}")
                num_filtered_systime += 1
                del data[i]
                continue

    num_filtered: int = num_filtered_gnss + num_filtered_depth + num_filtered_systime
    pct_filt_gnss = num_filtered_gnss / num_packets_gnss
    pct_filt_depth = num_filtered_depth / num_packets_depth
    pct_filt_systime = num_filtered_systime / num_packets_systime
    lineage.add_algorithm_element(name=ALG_NAME, parameters=params, source=SOURCE, version=__version__,
                                  comment=
                                  (f"Filtered {num_filtered} packets of {num_packets} total. "
                                   f"Filtered {pct_filt_gnss:.2%} of GNSS packets. "
                                   f"Filtered {pct_filt_depth:.2%} of Depth packets. "
                                   f"Filtered {pct_filt_systime:.2%} of SystemTime packets."
                                   )
                                  )

    return data


class NoDataReject(WiblAlgorithm):
    name: str = ALG_NAME
    phases: AlgorithmPhase = AlgorithmPhase.ON_LOAD

    @classmethod
    def run_on_load(cls,
                    data: List[LoggerFile.DataPacket],
                    params: str,
                    lineage: Lineage,
                    verbose: bool) -> List[LoggerFile.DataPacket]:
        return reject_nodata(data, params, lineage, verbose)
