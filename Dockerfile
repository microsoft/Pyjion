FROM ubuntu:16.04

RUN apt-get update -y && apt-get install -y wget

RUN echo "deb http://llvm.org/apt/xenial/ llvm-toolchain-xenial-3.8 main" | tee /etc/apt/sources.list.d/llvm.list
RUN wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key | apt-key add -

RUN apt-get update -y && apt-get install -y \
    git \
    cmake \
    llvm-3.9 \
    clang-3.9 \
    lldb-3.6 \
    lldb-3.6-dev \
    libunwind8 \
    libunwind8-dev \
    gettext \
    libicu-dev \
    liblttng-ust-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    uuid-dev

RUN mkdir /opt/pyjion
ADD . /opt/pyjion
WORKDIR /opt/pyjion

RUN echo fs.file-max = 100000 > /etc/sysctl.conf
RUN sysctl -p

CMD /bin/bash
