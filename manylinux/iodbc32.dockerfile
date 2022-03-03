FROM fdabrandao/manylinux32:coin_cmake-3.12.4
WORKDIR "/tmp"
RUN curl -sL https://github.com/openlink/iODBC/releases/download/v3.52.15/libiodbc-3.52.15.tar.gz | tar xz
WORKDIR "/tmp/libiodbc-3.52.15"
RUN ./configure && make && make install
