FROM phusion/baseimage:latest

# Environment variables
ENV HOME /root
ENV DEBIAN_FRONTEND noninteractive
ENV TERM linux

# Do common baseimage actions
RUN echo "/root" > /etc/container_environment/HOME && \
    echo "noninteractive" > /etc/container_environment/DEBIAN_FRONTEND && \
    echo "linux" > /etc/container_environment/TERM && \
    rm -f /etc/service/sshd/down && \
    /usr/sbin/enable_insecure_key && \
    /etc/my_init.d/00_regen_ssh_host_keys.sh

# Install necessary packages
RUN apt-get -qq update && \
    apt-get -qq install -y --no-install-recommends \
        sudo \
        git \
        vim \
        nano \
        curl \
        wget && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Add vagrant user
RUN useradd -m -s /bin/bash -p vagrant vagrant \
  && usermod -a -G sudo vagrant \
  && echo "vagrant ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/vagrant_user

# Add Vagrant key
RUN cd ~vagrant \
  && mkdir .ssh \
  && curl -sL https://raw.githubusercontent.com/mitchellh/vagrant/master/keys/vagrant.pub > .ssh/authorized_keys \
  && chown -R vagrant:vagrant .ssh \
  && chmod 0700 .ssh \
  && chmod 0600 .ssh/authorized_keys

# Cleanups
RUN rm -rf /tmp/* /var/tmp/*

# Init process is entrypoint
ENTRYPOINT ["/sbin/my_init", "--"]

