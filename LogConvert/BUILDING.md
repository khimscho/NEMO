# Instructions for building LogConvert

## Amazon Linux

### AMD64
```shell
$ docker compose -f docker-compose-amd64.yml run --remove-orphans logconvert-amd64
```

Binary will be created at `./build-x86_64/src/logconvert`. 
See [Dockerfile](./Dockerfile) for runtime dependencies.

### ARM64
```shell
$ docker compose -f docker-compose-arm64.yml run --remove-orphans logconvert-arm64
```

Binary will be created at `./build-aarch64/src/logconvert`.
See [Dockerfile](./Dockerfile) for runtime dependencies.
