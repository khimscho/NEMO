# Python code WIBL low-cost data logger system

## Installation

### Locally
```
pip install .
```

## Usage
```
]$ wibl --help
usage: wibl <command> [<arguments>]

    Commands include:
        datasim     Generate test data using Python-native data simulator.
        editwibl   Edit WIBL logger files, e.g., add platform metadata.


Python tools for WIBL low-cost data logger system

positional arguments:
  command     Subcommand to run

optional arguments:
  -h, --help  show this help message and exit
  --version   print version and exit
```

### Edit WIBL file
Add platform metadata to existing binary WIBL file (e.g., from data simulator or from a real datalogger):
```
$ wibl editwibl -m sensor-inject.json test.bin test-inject.bin
```
