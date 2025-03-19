FROM ubuntu:bionic

ENV DEBIAN_FRONTEND noninteractive
ENV WIX_SHA 74118792afa212edcbafd6bbcad9abd5a7c3cf6a8a884b2e2e787b29df4cbb88
ENV WIX_URL https://github.com/sk1project/wixpy/releases/download/v0.1/python-wix.py-0.1_ubuntu_18.04_amd64.deb

RUN apt-get update

RUN apt-get install -y gir1.2-libmsi-1.0 python-gi gir1.2-gcab libmsi0 libgsf-1-114  libgcab-1.0 libgsf-1-common wget

RUN wget -O /tmp/wix.deb $WIX_URL && test `sha256sum /tmp/wix.deb  | awk '{print $1}'` = $WIX_SHA && dpkg -i /tmp/wix.deb

VOLUME /package

CMD cd /package && wix.py wix.json
