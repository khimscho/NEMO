FROM debian:12-slim as builder

WORKDIR /gmt

RUN apt-get update -y
RUN apt-get install -y build-essential python3.11-dev gdal-bin libgdal-dev python3-gdal gmt libgmt-dev \
    python3-pip rasterio python3-rioxarray
RUN pip3.11 install --break-system-packages pygmt

FROM builder as final

CMD ["/bin/bash"]
