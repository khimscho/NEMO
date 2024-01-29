# Instructions for building LogConvert

## Amazon Linux 2023
Use the provided [Dockerfile](Dockerfile) as a starting point for building and running LogConvert
in Amazon Linux 2023 environments (e.g., public.ecr.aws/lambda/python:3.12).

### AMD64
```shell
$ docker buildx build --platform linux/amd64 -t wibl/logconvert:latest ./
```

### ARM64
```shell
$ docker buildx build --platform linux/arm64 -t wibl/logconvert:latest ./
```

