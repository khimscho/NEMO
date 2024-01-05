FROM public.ecr.aws/amazonlinux/amazonlinux:2 as builder

RUN yum update -y && \
    yum install -y wget tar gzip cmake3 ninja-build gcc gcc-c++ boost-devel boost-static
# Note: for runtime, only boost-program-options should be needed.

# Install newer CMake as Amazon Linux's version is too old
WORKDIR /tmp/cmake
RUN wget https://github.com/Kitware/CMake/releases/download/v3.27.8/cmake-3.27.8-linux-$(uname -m).sh
RUN bash cmake-3.27.8-linux-$(uname -m).sh --skip-license

# Install NMEA200 library
WORKDIR /usr/src/NMEA2000
RUN wget https://github.com/ttlappalainen/NMEA2000/archive/refs/heads/master.tar.gz
RUN tar xf master.tar.gz
WORKDIR NMEA2000-master
RUN /tmp/cmake/bin/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -B build -S .
RUN /tmp/cmake/bin/cmake --build build -j $(nproc)
ENV N2K_INCLUDE=/usr/src/NMEA2000/NMEA2000-master/src
ENV N2K_LIB=/usr/src/NMEA2000/NMEA2000-master/build/src/libnmea2000.a

WORKDIR /usr/src/wibl/LogConvert

FROM builder AS final

CMD bash
