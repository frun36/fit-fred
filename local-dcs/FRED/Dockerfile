FROM cc7-fit

RUN mkdir fred
COPY core /fred/core

COPY mapi /fred/mapi
COPY CMakeLists.txt /fred

WORKDIR /fred/core
RUN source /opt/rh/devtoolset-7/enable && cmake3 -DMAPI=1 .
RUN make all

COPY boards /fred/boards
COPY /local-dcs/FRED/config /fred/config
COPY sections /fred/sections