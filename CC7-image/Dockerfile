FROM centos:centos7

COPY CentOS-Base.repo /etc/yum.repos.d/CentOS-Base.repo 
RUN yum clean all && yum makecache
RUN yum update -y

RUN yum install -y centos-release-scl

RUN sed -i s/mirror.centos.org/vault.centos.org/g /etc/yum.repos.d/CentOS-*.repo
RUN sed -i s/^#.*baseurl=http/baseurl=http/g /etc/yum.repos.d/CentOS-*.repo
RUN sed -i s/^mirrorlist=http/#mirrorlist=http/g /etc/yum.repos.d/CentOS-*.repo

RUN yum install -y epel-release

RUN sed -i s/mirror.centos.org/vault.centos.org/g /etc/yum.repos.d/CentOS-*.repo
RUN sed -i s/^#.*baseurl=http/baseurl=http/g /etc/yum.repos.d/CentOS-*.repo
RUN sed -i s/^mirrorlist=http/#mirrorlist=http/g /etc/yum.repos.d/CentOS-*.repo

RUN yum update -y
RUN yum install -y devtoolset-7 cmake3 libaio
RUN echo "source /opt/rh/devtoolset-7/enable" >> ~/.bashrc

