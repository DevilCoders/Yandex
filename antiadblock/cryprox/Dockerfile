FROM registry.yandex.net/antiadb/ubuntu-xenial/baseimage:9540589

WORKDIR /
COPY . /

RUN mkdir -p /usr/local/lib/lua/5.1/ && \
    ln -sf /usr/lib/x86_64-linux-gnu/lua/5.1/cjson.so /usr/local/lib/lua/5.1/ && \
    ln -sf /etc/nginx/sites-available/cryprox-ping-opened.conf /etc/nginx/sites-available/cryprox-ping-location.conf && \
    ln -s /etc/nginx/sites-available/static-mon.conf /etc/nginx/sites-enabled/static-mon.conf && \
    ln -s /etc/nginx/sites-available/cookiematcher.conf /etc/nginx/sites-enabled/cookiematcher.conf && \
    ln -s /etc/nginx/sites-available/cryprox.conf /etc/nginx/sites-enabled/cryprox.conf && \
    ln -s /etc/nginx/sites-available/naydex.conf /etc/nginx/sites-enabled/naydex.conf && \
    mv /etc/nginx/slb /usr/bin/slb && \
    chmod +x /etc/nginx/launch.sh /usr/bin/slb /log_shippers/push-client/error_pipe /juggler/checks/check.py /juggler/checks/bypass_uids_check.py && \
    chmod +x /log_shippers/filebeat/cryprox/launch_filebeat.sh /log_shippers/filebeat/nginx/launch_filebeat.sh /log_shippers/update_filebeat/update_filebeat
