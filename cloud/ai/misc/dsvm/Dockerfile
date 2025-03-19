FROM ubuntu:18.04

RUN apt-get update && apt-get install -y software-properties-common sudo wget curl systemd

COPY sync /sync/
WORKDIR /sync/

RUN ./setup.sh

RUN rm -rf /sync

RUN useradd -d /home/agorodilov -g root -G sudo -ms /bin/bash agorodilov
USER agorodilov
WORKDIR /home/agorodilov

