# Base container images for running WIBL in Docker
This page describes how to use WIBL base container imagers for running WIBL
in Docker or other container runtime environments.

## Usage
To run a `wibl-python` command within the container while reading and
writing data from a directory on your host computer, do the following:
```shell
$ docker run -v ./:/var/wibl -ti ghcr.io/wibl/python:1.0.3 'wibl datasim -f test.bin -d 360 -b'
...
INFO:wibl.command.datasim:Step to time: 359000000
INFO:wibl.command.datasim:Step to time: 359157800
INFO:wibl.command.datasim:Step to time: 360000000
INFO:wibl.command.datasim:Total iterations: 600
$ docker run -v ./:/var/wibl -ti ghcr.io/wibl/python:1.0.3 'wibl editwibl -m sensor-inject.json test.bin test-inject.bin'
...
```

where:
 - `-v ./:/var/wibl` mounts the current working directory from the host to `/var/wibl` in the container.
 - `'wibl datasim -f test.bin -d 360 -b'` is the `wibl-python` command to run in the container

> Note: the entire command to be run in the container must be enclosed in quotes

Since it can be error-prone to specify the full `docker run` command each
time you run a `wibl-python` command, it can be easier to open a shell
in the `wibl-base` container, then run multiple `wibl` commands:
```shell
$ docker run -v ./:/var/wibl -ti ghcr.io/wibl/python:1.0.3
bash-5.2# wibl datasim -f test3.bin -d 360 -b
...
INFO:wibl.command.datasim:Step to time: 359000000
INFO:wibl.command.datasim:Step to time: 359467302
INFO:wibl.command.datasim:Step to time: 360000000
INFO:wibl.command.datasim:Total iterations: 603
bash-5.2# wibl editwibl -m sensor-inject.json test.bin test-inject.bin 
...
bash-5.2# exit
exit
```

