FROM redhat/ubi8
RUN yum update -y
RUN yum install -y gcc-c++ cmake make
COPY . /fred

WORKDIR /fred
RUN cmake -DMAPI=1
RUN make
