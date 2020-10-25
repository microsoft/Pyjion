ARG DOTNET_VERSION=5.0.100-rc.2.20479.15
FROM ubuntu:latest
ARG DOTNET_VERSION
RUN echo "Building Pyjion with .NET  $DOTNET_VERSION"
COPY . /src
WORKDIR /src

ENV TZ=Europe/London
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get -y update && apt-get install -y software-properties-common && apt-get -y update \
    && add-apt-repository ppa:deadsnakes/ppa && apt-get -y update \
    && apt-get install -y wget cmake llvm-9 clang-9 autoconf automake \
     libtool build-essential curl ninja-build git lldb-6.0 liblldb-6.0-dev \
     libunwind8 libunwind8-dev gettext libicu-dev liblttng-ust-dev \
     libssl-dev libnuma-dev libkrb5-dev zlib1g-dev \
     && apt-get install -y python3.9 \
     && apt-get clean -y
RUN wget https://dotnetcli.azureedge.net/dotnet/Sdk/${DOTNET_VERSION}/dotnet-sdk-${DOTNET_VERSION}-linux-x64.tar.gz
RUN mkdir -p dotnet && tar zxf dotnet-sdk-${DOTNET_VERSION}-linux-x64.tar.gz -C dotnet
ENV DOTNET_ROOT=/src/dotnet
RUN cmake -E make_directory build && cmake -DCMAKE_BUILD_TYPE=Release && cmake --build . --config Release --target _pyjion
RUN python3.9 -m pip install .

