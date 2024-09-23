FROM cc7-fit
COPY . /fred

WORKDIR /fred/core
RUN source /opt/rh/devtoolset-7/enable && cmake3 -DMAPI=1 -S .. -B .
RUN make
