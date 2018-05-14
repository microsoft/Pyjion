FROM ubuntu:16.04

RUN apt-get update -y && apt-get install -y wget

RUN echo "deb http://llvm.org/apt/xenial/ llvm-toolchain-xenial-3.8 main" | tee /etc/apt/sources.list.d/llvm.list
RUN wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key | apt-key add -

RUN apt-get update -y && apt-get install -y \
    git \
    cmake \
    llvm-3.5 \
    clang-3.5 \
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

RUN git submodule update --init CoreCLR
RUN git submodule update --init Python
RUN git submodule update --init Tests/Catch
RUN echo fs.file-max = 100000 > /etc/sysctl.conf
RUN sysctl -p

WORKDIR /opt/pyjion/CoreCLR
# || true is because build.sh fails but all the stuff we care about is there.
RUN ./build.sh || true

WORKDIR /opt/pyjion/Python
RUN ./configure
RUN make

WORKDIR /opt/pyjion/Pyjion
RUN ./make.sh

CMD /opt/pyjion/Python/python
