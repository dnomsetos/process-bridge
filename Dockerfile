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
        python3 \
        python3-pip \
        python3-venv \
	libc6-dev:i386 \
        gcc-multilib \
        g++-multilib && \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:${PATH}"
RUN pip install --upgrade pip

CMD ["/bin/bash"]
