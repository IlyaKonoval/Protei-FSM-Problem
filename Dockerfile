FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . /src

RUN cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build-release -j \
    && cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug \
    && cmake --build build-debug -j

CMD ["bash"]
