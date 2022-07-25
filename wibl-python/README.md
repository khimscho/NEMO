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

## Packaging and deploying AWS lambda for DCDB processing

### Create IAM roles for lambda
Now, create an IAM role and the Lambda function via the AWS CLI.

First create the following trust policy JSON file

```bash
$ cat > trust-policy.json
{
 "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Principal": {
        "Service": ["lambda.amazonaws.com"]
      },
      "Action": "sts:AssumeRole"
    }
  ]
}

```
Then create the IAM role:

```bash
$ aws iam create-role --role-name wibl-dcdb-preprocess-lambda --assume-role-policy-document file://trust-policy.json
```

Note down the role Arn returned to you after running that command. We'll need it to create the lambda below.

Attach the following policy to allow Lambda to write logs in CloudWatch:
```bash
$ aws iam attach-role-policy --role-name wibl-dcdb-preprocess-lambda --policy-arn arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole
```

### Package lambda as zipfile
> Note: the following is based on [AWS lambda deployment instructions](https://docs.aws.amazon.com/lambda/latest/dg/python-package.html#python-package-create-package-with-dependency)

First, install WIBL and its dependencies into a target directory called `package` located in the current directory:
```bash
$ pip install --target ./package .
```

Next, make a zipfile of the `package` directory:
```bash
$ pushd .
$ cd package
$ zip -r ../wibl-dcdb-preprocess-lambda-package.zip .
$ popd
```

### Create the lambda
```bash
$ aws lambda create-function --function-name wibl-dcdb-preprocess \
--role arn:aws:iam::?????:role/wibl-dcdb-preprocess \
--runtime python3.9 --timeout 15 --memory-size 128 \
--handler wibl.processing.cloud.aws.lambda_function.lambda_handler \
--zip-file fileb://wibl-dcdb-preprocess-lambda-package.zip \
--environment "Variables={PROVIDER_ID=MY_PROVIDER_ID,DCDB_STAGING_BUCKET=MY_STAGING_BUCKET,UPLOAD_POINT=https://www.ngdc.noaa.gov/ingest-external/upload/csb/geojson/}"
```

where `arn:aws:iam::?????:role/wibl-dcdb-preprocess` is replaced by the lambda role ARN created above.

### Upload a new version of the lambda
```bash
$ aws lambda update-function-code --function-name wibl-dcdb-preprocess \
--zip-file fileb://wibl-dcdb-preprocess-lambda-package.zip
```

### Change the lambda environment
```bash
$ aws lambda update-function-configuration --function-name wibl-dcdb-preprocess \
--environment "Variables={PROVIDER_ID=MY_PROVIDER_ID_UPDATED,DCDB_STAGING_BUCKET=MY_STAGING_BUCKET_UPDATED,UPLOAD_POINT=https://www.ngdc.noaa.gov/ingest-external/upload/csb/geojson/}"
```
