FROM fred-server-env
COPY . /fred

WORKDIR /fred
RUN cmake -DMAPI=1 -Dtest-only=ON
RUN make
