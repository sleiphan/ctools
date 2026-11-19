FROM debian:latest AS base

RUN apt update
RUN apt install -y \
    gcc \
    g++ \
    gdb \
    make \
    cmake \
    git \
    gh \
    linux-libc-dev \
    clang-format

# Set locale to UTF-8
ENV LC_ALL='C.utf8'
