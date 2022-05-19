# Python code WIBL low-cost data logger system

## Installation

### Locally
```
pip install .
```

## Testing
To run unit tests, including detailed verification of packet data output by the data simulator, run:
```
pip install -r requirements-build.txt
pytest -n 4 --cov=wibl --cov-branch --cov-report=xml --junitxml=test-results.xml tests/unit/*.py
```

## Usage
```
]$ wibl --help
usage: wibl <command> [<arguments>]

    Commands include:
        datasim    Generate test data using Python-native data simulator.
        editwibl   Edit WIBL logger files, e.g., add platform metadata.


Python tools for WIBL low-cost data logger system

positional arguments:
  command     Subcommand to run

optional arguments:
  -h, --help  show this help message and exit
  --version   print version and exit
```

### Run WIBL data simulator
Run data simulator to generate test data and store it in a binary WIBL file:
```
$ wibl datasim -f test.bin -d 360 -s
```

For more information on simulator, use the `-h` option:
```
$ wibl datasim -h
usage: wibl [-h] [-f FILENAME] [-d DURATION] [-s] [-b] [--use_buffer_constructor] [-v]

Command line user interface for the NMEA data simulator.

optional arguments:
  -h, --help            show this help message and exit
  -f FILENAME, --filename FILENAME
                        Simulated data output filename
  -d DURATION, --duration DURATION
                        Duration (seconds) of the simulated data
  -s, --emit_serial     Write NMEA0183 simulated strings
  -b, --emit_binary     Write NMEA2000 simulated data packets
  --use_buffer_constructor
                        Use buffer constructor, rather than data constructor, for data packets. If not specified, data constructor will be used.
  -v, --verbose         Produce verbose output.
```

### Edit WIBL file
Add platform metadata to existing binary WIBL file (e.g., from data simulator or from a real datalogger):
```
$ wibl editwibl -m sensor-inject.json test.bin test-inject.bin
```
