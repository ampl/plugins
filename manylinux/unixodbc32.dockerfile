FROM fdabrandao/manylinux32:coin_cmake-3.12.4
WORKDIR "/tmp"
RUN curl -O http://www.unixodbc.org/unixODBC-2.3.9.tar.gz
RUN gunzip unixODBC-2.3.9.tar.gz
RUN tar xvf unixODBC-2.3.9.tar
WORKDIR "/tmp/unixODBC-2.3.9"
RUN ./configure
RUN make
RUN make install
