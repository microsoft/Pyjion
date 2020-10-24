FROM ubuntu:latest
WORKDIR /pyjion-home

ENV TZ=Europe/London
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get -y update && apt-get install -y wget cmake llvm-9 clang-9 autoconf automake \
     libtool build-essential python curl git lldb-6.0 liblldb-6.0-dev \
     libunwind8 libunwind8-dev gettext libicu-dev liblttng-ust-dev \
     libssl-dev libnuma-dev libkrb5-dev zlib1g-dev \
     && apt-get clean -y
RUN wget https://dotnetcli.azureedge.net/dotnet/Sdk/5.0.100-rc.1.20452.10/dotnet-sdk-5.0.100-rc.1.20452.10-linux-x64.tar.gz
RUN mkdir -p dotnet && tar zxf dotnet-sdk-5.0.100-rc.1.20452.10-linux-x64.tar.gz -C dotnet
RUN cmake -E make_directory build && cmake -DCMAKE_BUILD_TYPE=Debug && cmake --build . --config Debug
RUN ./unit_tests
