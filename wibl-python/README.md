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

First (and optionally), run a shell within the Amazon Linux Python 3.9 lambda runtime:
```bash
$ docker run --mount type=bind,source="$(pwd)/.",target=/build \
-it --entrypoint /bin/bash \
public.ecr.aws/lambda/python:3.9-x86_64
```

> Note: replace `-x86_64` with `-arm64` if your lambda will use ARM64 architecture instead of x86_64 (i.e., AMD64).

The reason you might want to package lambdas within Amazon Linux Python 3.9 lambda runtime is that it should be nearly
identical to the environment where the lambda will run. This way, you can reduce the risk of introducing any platform-
or architecture-specific dependencies. That said, since we are using AWS layers to configure some of the dependencies
for our lambdas (e.g., including platform- and architecture-specific dependencies such as numpy), and since the
dependencies we *are* bundlying in our lambda zipfile (as specified by
[requirements-lambda.txt](requirements-lambda.txt)) *should* be pure Python, it *should* be okay to create this
package on either non-Amazon versions of Linux, or under recent versions of macOS. However, it is probably a good idea
to use Amazon Linux Python 3.9 lambda runtime to package these lambdas.

Second, install WIBL and its dependencies into a target directory called `package` located in the current directory:
```bash
$ pip install --target ./package -r requirements-lambda.txt
$ pip install --target ./package --no-deps .
```

> Note: if building in Amazon Linux Python 3.9 lambda runtime, make sure to run these in the shell you started in
> Docker above rather than in a shell on your host OS.

> Note: we break out the install into two steps so that we package *only* the subset of `wibl` dependencies that will
> not be provided by AWS lambda layers used when we create the lambda below.

Next, make a zipfile of the `package` directory:
```bash
$ pushd .
$ cd package
$ zip -r ../wibl-package.zip .
$ popd
```

> Note: if building in Amazon Linux Python 3.9 lambda runtime, you'll have to run this from your host since `zip` is not
> installed by default.

### Create the processing lambda
```bash
$ aws lambda create-function --function-name wibl-dcdb-processing \
--role arn:aws:iam::?????:role/wibl-dcdb-processing-lambda \
--runtime python3.9 --timeout 15 --memory-size 128 \
--handler wibl.processing.cloud.aws.lambda_function.lambda_handler \
--zip-file fileb://wibl-package.zip \
--architectures x86_64 \
--layers LAYER1 ... LAYERN \
--environment "Variables={PROVIDER_ID=MY_PROVIDER_ID,STAGING_BUCKET=MY_STAGING_BUCKET,UPLOAD_POINT=https://www.ngdc.noaa.gov/ingest-external/upload/csb/geojson/}"
```

where `arn:aws:iam::?????:role/wibl-dcdb-processing-lambda` is replaced by the lambda role ARN created above,
and where `LAYER1 ... LAYERN` are replaced by the names of the AWS lambda layers we would like to deploy this lambda
on top of (to avoid repackaging dependencies).

> Note: by default, the lambda will use the configuration file installed as part of the `wibl` Python package. You can
> find this default file in [here](wibl/defaults/processing/cloud/aws/configure.json). If you want to use another file,
> make sure to add it to the `package` directory created above and set an environment variable named `WIBL_CONFIG_FILE`
> (value is the path within the package to the desired configuration file) when creating the lambda function.

> Note: if you want to use ARM64 instead of x86_64 architecture, change `--architectures x86_64` to
> `--architectures arm64`.

### Configure the processing lambda to trigger from S3
To have the processing lambda triggered by the upload of a file into an S3 bucket, you need to configure the lambda to be triggerable, configure the bucket to generate notifications (and send them to the lambda), and then configure the lambda to allow for access to the S3 bucket.  In order:

```bash
$ aws lambda add-permission --function-name wibl-dcdb-processing \
  --action lambda:InvokeFunction \
  --statement-id s3invoke \
  --principal s3.amazonaws.com \
  --source-arn arn:aws:s3:::INCOMING_BUCKET \
  --source-account ACCOUNT_NUMBER
```
where `INCOMING_BUCKET` is the name of the S3 bucket that you'll upload data into from the outside world, and `ACCOUNT_NUMBER` is the AWS account number for your installation.

> Note: Some services can invoke functions in other accounts. If you specify a source
> ARN that has your account ID in it, that isn't an issue. For Amazon S3, however, the
> source is a bucket whose ARN doesn't have an account ID in it. It's possible that you
> could delete the bucket and another account could create a bucket with the same name.
> Using the source-account option with your account ID ensures that only resources in
> your account can invoke the function.

Generate a configuration file for the bucket to notify the lambda:
```bash
$ cat > bucket-notification-cfg.json
{
    "LambdaFunctionConfigurations": [
        {
            "Id": "IDENTIFIER",
            "LambdaFunctionArn": "arn:aws:lambda:REGION:ACCOUNT_NUMBER:function:wibl-dcdb-processing",
            "Events": [
                "s3:ObjectCreated:Put"
            ]
        }
    ]
}
```
where `IDENTIFIER` is a unique identifier (e.g., a UUID), `REGION` is your AWS region, and `ACCOUNT_NUMBER` is as above.  You can obtain the ARN for your bucket directly from the AWS console if necessary.  Then apply the notification to the bucket:

```bash
$ aws s3api put-bucket-notification-configuration \
  --bucket INCOMING_BUCKET \
  --notification-configuration file://bucket-notification-cfg.json
```
where `INCOMING_BUCKET` is the short form name of the bucket you're using for incoming files, as above.

Finally, generate a configuration file to allow the lambda access to the S3 bucket:
```bash
$ cat > lambda-s3-access.json
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Sid": "LambdaAllowS3Access",
            "Effect": "Allow",
            "Action": [
                "s3:*"
            ],
            "Resource": [
                "arn:aws:s3:::INCOMING_BUCKET",
                "arn:aws:s3:::INCOMING_BUCKET/*"
            ]
        }
    ]
}
```
and apply it to the lambda:
```bash
$ aws iam put-role-policy --role-name wibl-dcdb-processing-lambda \
  --policy-name lambda-s3-access \
  --policy-document file://lambda-s3-access.json
```
and then convince yourself that it has been applied:
```bash
$ aws iam list-role-policies --role-name wibl-dcdb-process-lambda
```
which should report something like:
```
{
    "PolicyNames": [
        "lambda-s3-access"
    ]
}
```

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

> Note: the submission lambda takes an additional environment variable `PROVIDER_AUTH`, which is an authentication key
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
