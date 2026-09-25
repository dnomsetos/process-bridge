FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
    	curl \
        build-essential \
	clang \
	cmake \
        ninja-build \
        git \
	libc6-dev:i386 \
        gcc-multilib \
        g++-multilib && \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["/bin/bash"]
