FROM quay.io/pypa/manylinux2014_x86_64

ARG UNIXODBC_VERSION=2.3.14

WORKDIR /tmp
RUN curl -fsSLO https://www.unixodbc.org/unixODBC-${UNIXODBC_VERSION}.tar.gz \
 && tar xzf unixODBC-${UNIXODBC_VERSION}.tar.gz \
 && cd unixODBC-${UNIXODBC_VERSION} \
 && ./configure --prefix=/usr/local --sysconfdir=/etc \
 && make \
 && make install \
 && ldconfig \
 && cd /tmp && rm -rf unixODBC-${UNIXODBC_VERSION}*
