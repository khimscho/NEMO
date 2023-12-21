
# Steps for deploying and testing WIBL visualizion lambda locally using [localstack](https://www.localstack.cloud)

## Create input and output buckets
TBD

## Upload example data to input bucket
TBD

## Set variable
```shell
$ mkdir -p /tmp/vizlmambda
$ export WIBL_BUILD_LOCATION=/tmp/vizlmambda
```

## Build lambda Docker image
From the `wibl-python` directory of the `wibl` repo, run:
```shell
$ docker build -f Dockerfile.vizlambda -t vizlambda:latest .
```

## Push image to ECR
Log in to ECR:
```shell
$ aws ecr get-login-password --region us-east-1 | docker login \
  --username AWS --password-stdin 111122223333.dkr.ecr.us-east-1.amazonaws.com
```

Create repo:
```shell
$ aws ecr create-repository --repository-name hello-world \
  --region us-east-1 --image-scanning-configuration scanOnPush=true --image-tag-mutability MUTABLE
```

Tag local image using ECR repository URL:
```shell
$ docker tag docker-image:test 111122223333.dkr.ecr.us-east-1.amazonaws.com/hello-world:latest
```

Push image to ECR:
```shell
$ docker push 111122223333.dkr.ecr.us-east-1.amazonaws.com/hello-world:latest
```

## Test invoking the function
```shell
$ aws lambda invoke --function-name hello-world response.json
```
