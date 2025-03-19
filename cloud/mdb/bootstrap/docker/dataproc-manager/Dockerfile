FROM ubuntu:20.04

COPY build/grpc_health_probe /grpc_health_probe

COPY build/dataproc-manager /opt/yandex/dataproc-manager/
RUN useradd -ms /bin/bash dataproc-manager

USER dataproc-manager
