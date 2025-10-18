FROM gcc:latest AS builder

WORKDIR /workspace
COPY . .
RUN make bootstrap
