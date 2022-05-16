import io
from typing import BinaryIO
import sys
import argparse
import logging

from wibl.simulator.data import DataGenerator, Engine, CLOCKS_PER_SEC


logger = logging.getLogger(__name__)


def datasim():
    """
    Provides a very simple interface to the simulator for NMEA data so that files of a given
    size can be readily generated for testing data loggers and transfer software.
    :return:
    """
    parser = argparse.ArgumentParser(
        description='Command line user interface for the NMEA data simulator.'
    )
    parser.add_argument('-f', '--filename', help='Simulated data output filename')
    parser.add_argument('-d', '--duration', help='Duration (seconds) of the simulated data',
                        type=int)
    parser.add_argument('-s', '--emit_serial', help='Write NMEA0183 simulated strings',
                        action='store_true', default=True)
    parser.add_argument('-b', '--emit_binary', help='Write NMEA2000 simulated data packets',
                        action='store_true', default=False)
    parser.add_argument('-v', '--verbose', help='Produce verbose output.',
                        action='store_true', default=False)
    args = parser.parse_args(sys.argv[2:])

    if args.verbose:
        logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)
    else:
        logging.basicConfig(stream=sys.stdout, level=logging.INFO)

    outfile: BinaryIO = open(args.filename, 'wb')
    duration: int = args.duration * CLOCKS_PER_SEC

    gen: DataGenerator = DataGenerator(emit_nmea0183=args.emit_serial,
                                       emit_nmea2000=args.emit_binary)
    writer: io.BufferedWriter = io.BufferedWriter(outfile)
    engine: Engine = Engine(gen)

    first_time: int
    current_time: int

    current_time = first_time = engine.step_engine(writer)
    while current_time - first_time < duration:
        current_time = engine.step_engine(writer)
        logger.info(f"Step to time: {current_time}")
