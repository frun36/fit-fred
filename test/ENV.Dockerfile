FROM redhat/ubi8
RUN yum update -y
RUN yum install -y gcc-toolset-12 cmake make
RUN echo "source /opt/rh/gcc-toolset-12/enable" >> ~/.bashrc

