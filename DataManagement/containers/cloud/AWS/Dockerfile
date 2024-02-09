FROM public.ecr.aws/lambda/python:3.12 AS builder

RUN dnf -y install wget tar gzip bzip2 clang cmake ninja-build m4 boost-devel boost-static boost-program-options \
    amazon-efs-utils ghostscript pcre-devel glib2-devel lapack-devel openblas-devel fftw-devel libcurl-devel \
    libxml2-devel sqlite-devel expat-devel libcurl-devel zlib-devel libtiff-devel libpng-devel libjpeg-turbo-devel
# Note: for runtime, only boost-program-options should be needed.

# Checkout WIBL repo so that we can build LogConvert and install wibl-python
WORKDIR /usr/src/wibl
RUN wget https://bitbucket.org/ccomjhc/wibl/get/8f8122b00e709a4001bee6c2bd11c4c094b48f62.tar.gz \
    -O wibl-8f8122b00e709a4001bee6c2bd11c4c094b48f62.tar.gz
RUN echo "aa9e5aa6d107400612e04acfcfef9a6c7e4b0b55c6ba3a65ffd1f0b8f1548490  wibl-8f8122b00e709a4001bee6c2bd11c4c094b48f62.tar.gz" > wibl-8f8122b00e709a4001bee6c2bd11c4c094b48f62.tar.gz.sum
RUN sha256sum -c wibl-8f8122b00e709a4001bee6c2bd11c4c094b48f62.tar.gz.sum
RUN tar xf wibl-8f8122b00e709a4001bee6c2bd11c4c094b48f62.tar.gz
ENV WIBL_SRC=/usr/src/wibl/ccomjhc-wibl-8f8122b00e70

# Install NMEA200 library
WORKDIR /usr/src/NMEA2000
RUN wget https://github.com/ttlappalainen/NMEA2000/archive/383d7fb207bd70ad6e287ff424f41f048baff8cf.tar.gz \
    -O NMEA2000-383d7fb207bd70ad6e287ff424f41f048baff8cf.tar.gz
RUN echo "e61e9d7476ca6aa403e8d0981945bbcfe45173b69120c089b1a514ba520ec4a6  NMEA2000-383d7fb207bd70ad6e287ff424f41f048baff8cf.tar.gz" > NMEA2000-383d7fb207bd70ad6e287ff424f41f048baff8cf.tar.gz.sum
RUN sha256sum -c NMEA2000-383d7fb207bd70ad6e287ff424f41f048baff8cf.tar.gz.sum
RUN tar xf NMEA2000-383d7fb207bd70ad6e287ff424f41f048baff8cf.tar.gz
ENV NMEA2000_SRC=/usr/src/NMEA2000/NMEA2000-383d7fb207bd70ad6e287ff424f41f048baff8cf
WORKDIR $NMEA2000_SRC
RUN cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -B build -S .
RUN cmake --build build -j $(nproc)
ENV N2K_INCLUDE=${NMEA2000_SRC}/src
ENV N2K_LIB=${NMEA2000_SRC}/build/src/libnmea2000.a

# Build and install WIBL LogConvert
WORKDIR $WIBL_SRC/LogConvert
RUN cmake -G Ninja -Wno-dev -DCMAKE_BUILD_TYPE=Release -B build-$(uname -m) -S . \
  -DN2K_INCLUDE=${N2K_INCLUDE} -DN2K_LIB=${N2K_LIB}
RUN cmake --build build-$(uname -m) -j $(nproc)
RUN cp ./build-$(uname -m)/src/logconvert /usr/local/bin

# Setup wibl-python, including all dependencies needed to support visuzation using GMT
# Setup directories for code and source
RUN mkdir /tmp/code
RUN mkdir /tmp/src
RUN mkdir -p /var/gmt/data
ENV GMT_DATA=/var/gmt/data

# Download, build, and install GEOS
WORKDIR /tmp/src
RUN wget https://download.osgeo.org/geos/geos-3.12.1.tar.bz2
RUN echo "d6ea7e492224b51193e8244fe3ec17c4d44d0777f3c32ca4fb171140549a0d03  geos-3.12.1.tar.bz2" > geos-3.12.1.tar.bz2.sum
RUN sha256sum -c geos-3.12.1.tar.bz2.sum
RUN tar xf geos-3.12.1.tar.bz2
WORKDIR /tmp/src/geos-3.12.1
RUN mkdir build
WORKDIR /tmp/src/geos-3.12.1/build
RUN cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr && \
    cmake --build . -j && \
    cmake --build . --target install

# Download, build, and install PROJ
WORKDIR /tmp/src
RUN wget https://download.osgeo.org/proj/proj-9.3.1.tar.gz
RUN echo "b0f919cb9e1f42f803a3e616c2b63a78e4d81ecfaed80978d570d3a5e29d10bc  proj-9.3.1.tar.gz" > proj-9.3.1.tar.gz.sum
RUN sha256sum -c proj-9.3.1.tar.gz.sum
RUN tar xf proj-9.3.1.tar.gz
WORKDIR /tmp/src/proj-9.3.1
RUN mkdir build
WORKDIR /tmp/src/proj-9.3.1/build
RUN cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr && \
    cmake --build . -j && \
    cmake --build . --target install

# Download, build, and install HDF5
WORKDIR /tmp/src
RUN wget https://hdf-wordpress-1.s3.amazonaws.com/wp-content/uploads/manual/HDF5/HDF5_1_14_3/src/hdf5-1.14.3.tar.bz2
RUN echo "9425f224ed75d1280bb46d6f26923dd938f9040e7eaebf57e66ec7357c08f917  hdf5-1.14.3.tar.bz2" > hdf5-1.14.3.tar.bz2.sum
RUN sha256sum -c hdf5-1.14.3.tar.bz2.sum
RUN tar xf hdf5-1.14.3.tar.bz2
WORKDIR /tmp/src/hdf5-1.14.3
RUN mkdir build
WORKDIR /tmp/src/hdf5-1.14.3/build
RUN cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr && \
    cmake --build . -j && \
    cmake --build . --target install

# Download, build, and install NetCDF4
WORKDIR /tmp/src
RUN wget https://downloads.unidata.ucar.edu/netcdf-c/4.9.2/netcdf-c-4.9.2.tar.gz
RUN echo "cf11babbbdb9963f09f55079e0b019f6d0371f52f8e1264a5ba8e9fdab1a6c48  netcdf-c-4.9.2.tar.gz" > netcdf-c-4.9.2.tar.gz.sum
RUN sha256sum -c netcdf-c-4.9.2.tar.gz.sum
RUN tar xf netcdf-c-4.9.2.tar.gz
WORKDIR /tmp/src/netcdf-c-4.9.2
RUN mkdir build
WORKDIR /tmp/src/netcdf-c-4.9.2/build
RUN cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr && \
    cmake --build . -j && \
    cmake --build . --target install

# Download, build, and install libgeotiff
WORKDIR /tmp/src
RUN wget https://github.com/OSGeo/libgeotiff/releases/download/1.7.1/libgeotiff-1.7.1.tar.gz
RUN echo "05ab1347aaa471fc97347d8d4269ff0c00f30fa666d956baba37948ec87e55d6  libgeotiff-1.7.1.tar.gz" > libgeotiff-1.7.1.tar.gz.sum
RUN sha256sum -c libgeotiff-1.7.1.tar.gz.sum
RUN tar xf libgeotiff-1.7.1.tar.gz
WORKDIR /tmp/src/libgeotiff-1.7.1
RUN mkdir build
WORKDIR /tmp/src/libgeotiff-1.7.1/build
RUN cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr && \
    cmake --build . -j && \
    cmake --build . --target install

# Download, build, and install GDAL
WORKDIR /tmp/src
RUN wget https://github.com/OSGeo/gdal/releases/download/v3.8.2/gdal-3.8.2.tar.gz
RUN echo "e914690d2694671d75bab15b451a42c2f115d7267a5b600eb361e94b58a089d6  gdal-3.8.2.tar.gz" > gdal-3.8.2.tar.gz.sum
RUN sha256sum -c gdal-3.8.2.tar.gz.sum
RUN tar xf gdal-3.8.2.tar.gz
WORKDIR /tmp/src/gdal-3.8.2
RUN mkdir build
WORKDIR /tmp/src/gdal-3.8.2/build
# Don't build GDAL with -j (parallel) as it seems to break
RUN cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr && \
    cmake --build . -j && \
    cmake --build . --target install

# Download and install GMT support data
WORKDIR /tmp/src
RUN wget https://github.com/GenericMappingTools/dcw-gmt/releases/download/2.1.2/dcw-gmt-2.1.2.tar.gz
RUN tar xf dcw-gmt-2.1.2.tar.gz -C $GMT_DATA
ENV DCW_ROOT=$GMT_DATA/dcw-gmt-2.1.2
RUN wget https://github.com/GenericMappingTools/gshhg-gmt/releases/download/2.3.7/gshhg-gmt-2.3.7.tar.gz
RUN tar xf gshhg-gmt-2.3.7.tar.gz -C $GMT_DATA
ENV GSHHG_ROOT=$GMT_DATA/gshhg-gmt-2.3.7

# Download, build, and install GMT
WORKDIR /tmp/src
RUN wget https://github.com/GenericMappingTools/gmt/releases/download/6.5.0/gmt-6.5.0-src.tar.gz
RUN echo "b17e165fd6c85aeb0a281700bd89522af8c2676a2d7bdb51a6b242fa9f1779c9  gmt-6.5.0-src.tar.gz" > gmt-6.5.0-src.tar.gz.sum
RUN sha256sum -c gmt-6.5.0-src.tar.gz.sum
RUN tar xf gmt-6.5.0-src.tar.gz
WORKDIR /tmp/src/gmt-6.5.0
RUN mkdir build
WORKDIR /tmp/src/gmt-6.5.0/build
RUN cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_INSTALL_PREFIX=/usr \
      -DDCW_ROOT=$DCW_ROOT -DGSHHG_ROOT=$GSHHG_ROOT && \
    cmake --build . -j && \
    cmake --build . --target install

# Install Python dependencies for wibl-python and wibl-python
WORKDIR $WIBL_SRC/wibl-python
RUN pip install -r requirements-vizlambda.txt
RUN pip install ./wibl-manager
RUN pip install --no-deps .

# Clean-up
RUN rm -rf /usr/src/NMEA2000 /usr/src/wibl
RUN dnf -y clean all

FROM builder AS final

ENTRYPOINT ["/bin/bash"]
