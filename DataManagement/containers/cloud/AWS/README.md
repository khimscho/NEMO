# Base container image for running WIBL in AWS lambda environments

## Build
To build, do the following:
```shell
$ docker buildx build --platform linux/amd64,linux/arm64 \
  -t wibl-base-aws-lambda:0.1.0 .
```