FROM fit-fred-env
COPY . /fred

WORKDIR /fred/core
RUN source /opt/rh/gcc-toolset-12/enable && cmake -DMAPI=1 -S .. -B .
RUN make
