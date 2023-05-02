# Python code WIBL low-cost data logger system

## Installation

### Locally
```
pip install csbschema==1.0.4
pip install ./wibl-manager
pip install .
```

## Testing
To run unit tests, including detailed verification of packet data output by the data simulator, run:
```
pip install -r requirements-build.txt
pytest -n 4 --cov=wibl --cov-branch --cov-report=xml --junitxml=test-results.xml tests/unit/*.py
```

To run integration tests exercising much of the functionality of the `wibl` command line tool 
(except for file upload to S3 and submission DCDB):
```
bash ./tests/integration/test_wibl.bash
```

## Usage
```
]$ wibl --help
usage: wibl <command> [<arguments>]

    Commands include:
        datasim     Generate test data using Python-native data simulator.
        editwibl    Edit WIBL logger files, e.g., add platform metadata.
        uploadwibl  Upload WIBL logger files to an S3 bucket.
        parsewibl   Read and report contents of a WIBL file.
        dcdbupload  Upload a GeoJSON file to DCDB direct.
        

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

### Process WIBL file into GeoJSON
Convert a binary WIBL file into GeoJSON:
```
$ wibl procwibl -c tests/fixtures/configure.local.json test-inject.bin test-inject.geojson
```

> Note: It is necessary to supply a configuration JSON file with the `local` attribute
> set to `true`, such as `tests/fixtures/configure.local.json`, because `procwibl` uses
> the same code as the conversion processing lambda code run in the cloud.

### Upload WIBL files into AWS S3 Buckets for processing
Instead of using the mobile app (and for testing), WIBL binary files can be uploaded into a given S3 bucket to trigger processing.  If the file is being uploaded into the staging bucket (i.e., to test transfer to DCDB), a '.json' extension must be added (``-j|--json``), and the SourceID tag must be set (``-s|--source``) so that the submission Lambda can find this information.
```
$ wibl uploadwibl -h
usage: wibl uploadwibl [-h] [-b BUCKET] [-j] [-s SOURCE] input

Upload WIBL logger files to an S3 bucket (in a limited capacity)

positional arguments:
  input                 WIBL format input file

options:
  -h, --help            show this help message and exit
  -b BUCKET, --bucket BUCKET
                        Set the upload bucket name (string)
  -j, --json            Add .json extension to UUID for upload key
  -s SOURCE, --source SOURCE
                        Set SourceID tag for the S3 object (string)
```

### Parse WIBL binary files
Raw WIBL binary files can be read and transcribed in ASCII format for debug.  Statistics on which NMEA2000 talkers, and which packets, are observed can be dumped at the end of the read (``-s|--stats``).
```
$ wibl parsewibl -h
usage: wibl parsewibl [-h] [-s] input

Parse WIBL logger files locally and report contents.

positional arguments:
  input        Specify the file to parse

options:
  -h, --help   show this help message and exit
  -s, --stats  Provide summary statistics on packets seen
```

### Uploading GeoJSON files to DCDB directly
Instead of using the cloud-based submission process (and for debugging), pre-formatted GeoJSON files can be uploaded directly to DCDB for archival.  Note that the file containing the provider authorisation token (provided by DCDB for each Trusted Node) has to be set on the command line (``-a|--auth``), although the rest of the information (provider ID, upload point) can be specified through the JSON configuration file (``-c|--config``); the provider ID (specified by DCDB for each Trusted Node) can be over-ridden on the command line (``-p|--provider``).  The source ID is read from the GeoJSON file, unless it is set on the command line (``-s|--source``).

```
wibl dcdbupload -h
usage: wibl dcdbupload [-h] [-s SOURCEID] [-p PROVIDER] [-a AUTH] [-c CONFIG] input

Upload GeoJSON files to DCDB for archival.

positional arguments:
  input                 GeoJSON file to upload to DCDB

optional arguments:
  -h, --help            show this help message and exit
  -s SOURCEID, --sourceid SOURCEID
                        Set SourceID as in the GeoJSON metadata.
  -p PROVIDER, --provider PROVIDER
                        Provider ID as generated by DCDB
  -a AUTH, --auth AUTH  File containing the Provider authorisation token generated by DCDB
  -c CONFIG, --config CONFIG
                        Specify configuration file for installation
```

## Packaging and Deploying Processing and Submission Code
Packaging up the software for the cloud segment, and deploying it, can be a little involved due to security concerns with the various cloud providers.  Detailed setup instructions, and automation scripts, are provided as described below.

### AWS
There are a number of steps required to set up the AWS Lambdas, the S3 buckets, and associated triggers and permissions in order to make processing work in the cloud.  The steps are covered in detail in the [Setup Instructions](scripts/cloud/AWS/README.md), and a corresponding set of scripts are available in the [scripts](scripts/cloud/AWS) directory in the repository.  These scripts should *mostly* work, but will likely need some modification for a local configuration before being fully executable.  Consider the `configuration-parameters.sh` file first for this.

## Configuration for Local Use
The ``wibl`` tool can be installed locally as above, but needs a configuration JSON file for some of the sub-commands (e.g., to send data to DCDB) rather than using environment variables as with the cloud-based processing.  Scripts in `wibl-python/scripts/desktop` will generate the `configure/json` required (configure `configuration-parameters.sh` and then run `configure-desktop.sh`).
