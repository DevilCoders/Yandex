#!/bin/bash 

wget https://www.python.org/ftp/python/3.9.2/Python-3.9.2.tgz -P /var/tmp
tar -xf /var/tmp/Python-3.9.2.tgz -C /var/tmp/
cd /var/tmp/Python-3.9.2
./configure --enable-optimizations
make altinstall
