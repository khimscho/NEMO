FROM public.ecr.aws/amazonlinux/amazonlinux:2023 as builder

RUN dnf -y install wget tar gzip cmake ninja-build gcc gcc-c++ boost-devel boost-static boost-program-options
# Note: for runtime, only boost-program-options should be needed.

# Install NMEA200 library
WORKDIR /usr/src/NMEA2000
RUN wget https://github.com/ttlappalainen/NMEA2000/archive/refs/heads/master.tar.gz -O NMEA2000-master.tar.gz
RUN echo "57858a67e90af1b684c4cccfc6523de7240b64e5234e269816c349712f6cb712  NMEA2000-master.tar.gz" > NMEA2000-master.tar.gz.sum
RUN sha256sum -c NMEA2000-master.tar.gz.sum
RUN tar xf NMEA2000-master.tar.gz
WORKDIR NMEA2000-master
RUN cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -B build -S .
RUN cmake --build build -j $(nproc)
ENV N2K_INCLUDE=/usr/src/NMEA2000/NMEA2000-master/src
ENV N2K_LIB=/usr/src/NMEA2000/NMEA2000-master/build/src/libnmea2000.a

# Build and install WIBL LogConvert
WORKDIR /usr/src/wibl
RUN wget https://bitbucket.org/ccomjhc/wibl/get/9ed4f25ce65f.tar.gz -O wibl-9ed4f25ce65f.tar.gz
RUN echo "8690b09d00b7f27b67063b0c172d0c21c608ea9216710cd81f32fd3b877b88cf  wibl-9ed4f25ce65f.tar.gz" > wibl-9ed4f25ce65f.tar.gz.sum
RUN sha256sum -c wibl-9ed4f25ce65f.tar.gz.sum
RUN tar xf wibl-9ed4f25ce65f.tar.gz
WORKDIR ccomjhc-wibl-9ed4f25ce65f/LogConvert
RUN cmake -G Ninja -Wno-dev -DCMAKE_BUILD_TYPE=Release -B build-$(uname -m) -S . \
  -DN2K_INCLUDE=${N2K_INCLUDE} -DN2K_LIB=${N2K_LIB}
RUN cmake --build build-$(uname -m) -j $(nproc)
RUN cp ./build-$(uname -m)/src/logconvert /usr/local/bin

# Clean-up
RUN rm -rf /usr/src/NMEA2000 /usr/src/wibl
RUN dnf -y remove wget tar gzip cmake ninja-build gcc gcc-c++ boost-devel boost-static && \
    dnf -y autoremove && \
    dnf -y clean all

FROM builder AS final

CMD bash
