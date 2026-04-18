FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN cmake -S . -B /app/build -DCMAKE_BUILD_TYPE=Debug \
    && cmake --build /app/build --parallel

ENTRYPOINT ["/app/build/app"]
