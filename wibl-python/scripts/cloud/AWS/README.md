# WIBL AWS processing setup instructions and usage
The following instructions describe how to setup WIBL processing lambdas and WIBL manager
web service in AWS.

## Setup

### 1. Edit configuration
Edit `configuration-parameters.sh` as needed. The main thing you will probably need to do change
is the `DCDB_PROVIER_ID` variable. Once you do this, make sure your DCDB authorization key
is placed in a file named `ingest-external-${DCDB_PROVIDER_ID}.txt` in the same directory as
`configuration-parameters.sh`.

### 2. Create S3 buckets
Run `configure-buckets.sh` to create S3 buckets for incoming WIBL files and staging/outgoing
GeoJSON files:
```shell
$ ./configure-buckets.sh
```

### 3. Create SNS topics
Run `configure-sns.sh` to create SNS topics that will be used to orchestrate WIBL file processing
lambdas:
```shell
$ ./configure-sns.sh
```

### 4. Configure WIBL manager ECS service
Run `configure-manager-ecs.sh` to create WIBL manager ECS service. This will also create all VPC
resources needed to run WIBL processing lambdas:
```shell
$ ./configure-manager-ecs.sh
```

### 5. Configure WIBL processing lambdas
Run `configure-lambdas.sh` to create WIBL processing lambdas and SNS subscriptions needed to 
trigger them:
```shell
$ ./configure-lambdas.sh
```

#### Update WIBL processing lambda code
Run `update-lambda-code.sh` to update code for the WIBL processing lambdas after they 
have been created:
```shell
$ ./update-lambda-code.sh
```

### 6. Configure test EC2 instances (Optional)
Run `configure-ec2-test-instance.sh` to create EC2 instances on the private and public subnet
of the WIBL VPC. These EC2 instances can use useful for testing and debugging WIBL manager and 
processing service lambdas:
```shell
$ ./configure-ec2-test-instance.sh
```

## Usage

1. First upload a WIBL file into the WIBL incoming S3 bucket, for example a WIBL file named 
`6FF5B14E-65F3-4BFD-85D1-50F8CA02BC0B.wibl`.

2. Then trigger WIBL file conversion by issuing an HTTPS request to the conversion start lambda:
```shell
curl -v \
'https://fdjofdjofewjofjo5546jogfljfdlmfl3368.lambda-url.us-east-2.on.aws/' \
-H 'Content-Type: application/json' \
-d '{"object": "6FF5B14E-65F3-4BFD-85D1-50F8CA02BC0B.wibl"}'
```

> Note the URL of the conversion start lambda can be obtained from the `$CONVERSION_START_URL` environment variable
> within `configure-lambdas.sh`, or by running: 
> `echo "$(cat ${WIBL_BUILD_LOCATION}/create_url_lambda_conversion_start.json | jq -r '.FunctionUrl')".` 

This will result in the length of the file being returned via the HTTP response:
```
content-length: 63641
```

Further, if successful, the conversion start lambda will publish a message to the conversion SNS topic, which
will cause the conversion lambda to be invoked.
