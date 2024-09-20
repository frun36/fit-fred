FROM fred-server-env
COPY . /fred

WORKDIR /fred

RUN echo "source /opt/rh/gcc-toolset-12/enable" >> ~/.bashrc
RUN source /opt/rh/gcc-toolset-12/enable && cmake -DMAPI=1 -Dtest-only=ON
RUN make
