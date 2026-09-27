FROM debian:bookworm

ENV DEBIAN_FRONTEND=noninteractive

RUN set -eux; \
    arch="$(dpkg --print-architecture)"; \
    dpkg --add-architecture "$([ "$arch" = "amd64" ] && echo i386 || echo amd64)"; \
    apt-get update; \
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
        libc6-dev \
        gcc-multilib \
        g++-multilib \
        "libc6-dev:$([ "$arch" = "amd64" ] && echo i386 || echo amd64)"; \
    rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:${PATH}"

RUN pip install --upgrade pip

CMD ["/bin/bash"]
