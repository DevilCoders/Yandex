#!/bin/sh

#TODO: write instruction

#prepare symlinks
echo 'start make dirs and symlinks'
mkdir -p /logs/nginx
mkdir -p /bin/cryprox_run
mkdir /perm
mkdir -p /etc/cookiematcher
cp -r ./etc/nginx /etc
cp -r ./bin/cryprox_run/cryprox_run /bin/cryprox_run/cryprox_run
cp -r ./usr/bin/update_resources.py /usr/bin/update_resources.py

mkdir -p /usr/local/lib/lua/5.1/ && \
    ln -sf /usr/lib/x86_64-linux-gnu/lua/5.1/cjson.so /usr/local/lib/lua/5.1/ && \
    ln -sf /etc/nginx/sites-available/cryprox-ping-opened.conf /etc/nginx/sites-available/cryprox-ping-location.conf && \
    ln -s /etc/nginx/sites-available/static-mon.conf /etc/nginx/sites-enabled/static-mon.conf && \
    ln -s /etc/nginx/sites-available/cookiematcher.conf /etc/nginx/sites-enabled/cookiematcher.conf && \
    ln -s /etc/nginx/sites-available/cryprox.conf /etc/nginx/sites-enabled/cryprox.conf && \
    ln -s /etc/nginx/sites-available/naydex.conf /etc/nginx/sites-enabled/naydex.conf && \
    mv /etc/nginx/slb /usr/bin/slb && \
    chmod +x /usr/bin/slb
echo 'finished make dirs and symlinks'

#mount_tmpfs
echo 'start mount_tmpfs'
if [ ! -e /tmp_uids ]; then
  echo 'creating tmp_uids'
  mkdir -p /tmp_uids
  echo 'tmp_uids created'
  mount -t tmpfs -o size=5G,nr_inodes=1k,mode=0777 tmpfs /tmp_uids
  echo 'mount completed'
  echo 'download sandbox resources'
  /usr/bin/update_resources.py > /logs/update_resources.log
  echo 'finish download sandbox resources'
  df -h
fi
echo 'finished mount_tmpfs'

#run cryprox
echo 'start cryprox_run'
/bin/cryprox_run/cryprox_run &
