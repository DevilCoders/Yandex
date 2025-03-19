FROM registry.yandex.net/ubuntu:precise
RUN echo 'APT::Install-Recommends "0"; \n\
APT::Get::Assume-Yes "true"; \n\
APT::Get::force-yes "true"; \n\
APT::Install-Suggests "0";' > /etc/apt/apt.conf.d/01buildconfig && \
    apt-get update && \
    apt-get -y install binutils python-pip python-setuptools \
    libpython2.7 python-dev python-apt python-argparse upx-ucl && \
    pip install PyInstaller
