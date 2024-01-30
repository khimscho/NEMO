

# Build gmt image
```shell
docker build -t ccom-gmt:0.0.1-1 .
```


# Running gmt
```shell
docker run --rm --name gmt \
  --mount type=bind,src=./,dst=/gmt \
  -it ccom-gmt:0.0.1-1
```