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

## Packaging and deploying AWS lambdas for DCDB processing

### Create IAM roles for WIBL lambdas
First, create an IAM role for each lambda (e.g., processing and submission). We'll do this using the AWS CLI rather
than the AWS web console since this enables us to automate lambda configuration. Also, the CLI tends to change less
often than the web console user interface, so these instructions should last much longer without updating.

First create the following trust policy JSON file:
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
Then create the IAM role for the processing lambda:
```bash
$ aws iam create-role --role-name wibl-dcdb-processing-lambda --assume-role-policy-document file://trust-policy.json
```

Next, create the IAM role for the submission lambda:
```bash
$ aws iam create-role --role-name wibl-dcdb-submission-lambda --assume-role-policy-document file://trust-policy.json
```

> Make a copy of the role ARNs returned to you after running these commands. We'll need them to create the lambdas below.

Attach the following policy to allow Lambda to write logs in CloudWatch, processing lambda:
```bash
$ aws iam attach-role-policy --role-name wibl-dcdb-processing-lambda --policy-arn arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole
```

also for submission lambda:
```bash
$ aws iam attach-role-policy --role-name wibl-dcdb-submission-lambda --policy-arn arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole
```

### Package lambdas as zipfile
> Note: the following is based on [AWS lambda deployment instructions](https://docs.aws.amazon.com/lambda/latest/dg/python-package.html#python-package-create-package-with-dependency)

Since all WIBL AWS lambdas are part of the same Python package called `wibl`, we'll only need to create a single
package. We can specify different lambda handler functions when creating the individual lambdas below.

First, install WIBL and its dependencies into a target directory called `package` located in the current directory:
```bash
$ pip install --target ./package .
```

Next, make a zipfile of the `package` directory:
```bash
$ pushd .
$ cd package
$ zip -r ../wibl-package.zip .
$ popd
```

### Create the processing lambda
```bash
$ aws lambda create-function --function-name wibl-dcdb-processing \
--role arn:aws:iam::?????:role/wibl-dcdb-processing-lambda \
--runtime python3.9 --timeout 15 --memory-size 128 \
--handler wibl.processing.cloud.aws.lambda_function.lambda_handler \
--zip-file fileb://wibl-package.zip \
--environment "Variables={PROVIDER_ID=MY_PROVIDER_ID,STAGING_BUCKET=MY_STAGING_BUCKET,UPLOAD_POINT=https://www.ngdc.noaa.gov/ingest-external/upload/csb/geojson/}"
```

where `arn:aws:iam::?????:role/wibl-dcdb-processing-lambda` is replaced by the lambda role ARN created above.

> Note: by default, the lambda will use the configuration file installed as part of the `wibl` Python package. You can
> find this default file in [here](wibl/defaults/processing/cloud/aws/configure.json). If you want to use another file,
> make sure to add it to the `package` directory created above and set an environment variable named `WIBL_CONFIG_FILE`
> (value is the path within the package to the desired configuration file) when creating the lambda function.

### Create the submission lambda
```bash
$ aws lambda create-function --function-name wibl-dcdb-submission \
--role arn:aws:iam::?????:role/wibl-dcdb-submission-lambda \
--runtime python3.9 --timeout 15 --memory-size 128 \
--handler wibl.submission.cloud.aws.lambda_function.lambda_handler \
--zip-file fileb://wibl-package.zip \
--environment "Variables={PROVIDER_ID=MY_PROVIDER_ID,PROVIDER_AUTH=MY_PROVIDER_AUTH_KEY,STAGING_BUCKET=MY_STAGING_BUCKET,UPLOAD_POINT=https://www.ngdc.noaa.gov/ingest-external/upload/csb/geojson/}"
```

where `arn:aws:iam::?????:role/wibl-dcdb-submission-lambda` is replaced by the lambda role ARN created above.

> Note: The submission lambda takes an additional environment variable `PROVIDER_AUTH`, which is an authentication key
> used for making submissions to the HTTP service denoted by `UPLOAD_POINT`.

> Note: by default, the lambda will use the configuration file installed as part of the `wibl` Python package. You can
> find this default file in [here](wibl/defaults/submission/cloud/aws/configure.json). If you want to use another file,
> make sure to add it to the `package` directory created above and set an environment variable named `WIBL_CONFIG_FILE`
> (value is the path within the package to the desired configuration file) when creating the lambda function.

### Upload a new version of a lambda
```bash
$ aws lambda update-function-code --function-name wibl-dcdb-processing \
--zip-file fileb://wibl-package.zip
```

### Change lambda environment
```bash
$ aws lambda update-function-configuration --function-name wibl-dcdb-submission \
--environment "Variables={PROVIDER_ID=MY_PROVIDER_ID_UPDATED,PROVIDER_AUTH=MY_PROVIDER_AUTH_KEY_UPDATED,STAGING_BUCKET=MY_STAGING_BUCKET_UPDATED,UPLOAD_POINT=https://www.ngdc.noaa.gov/ingest-external/upload/csb/geojson/}"
```
