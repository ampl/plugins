FROM quay.io/pypa/manylinux2014_x86_64

ARG IODBC_VERSION=3.52.16

WORKDIR /tmp
RUN curl -fsSLO https://github.com/openlink/iODBC/releases/download/v${IODBC_VERSION}/libiodbc-${IODBC_VERSION}.tar.gz \
 && tar xzf libiodbc-${IODBC_VERSION}.tar.gz \
 && cd libiodbc-${IODBC_VERSION} \
 && ./configure --prefix=/usr/local --disable-gui --with-iodbc-inidir=/etc \
 && make -j"$(nproc)" \
 && make install \
 && ldconfig \
 && cd /tmp && rm -rf libiodbc-${IODBC_VERSION}*
