ARG REGISTRY=crptqd44ecjqf9hd0p9t
FROM cr.yandex/${REGISTRY}/ubuntu:20.04
WORKDIR /
RUN mkdir -p /logs
RUN mkdir -p /bin
RUN mkdir -p /conf
RUN mkdir -p /migrations
RUN mkdir -p /migrations_common
RUN mkdir -p /root/.postgresql
COPY postgresql_root.crt /root/.postgresql/root.crt
RUN chmod 0600 /root/.postgresql/root.crt
COPY /sources.list /etc/apt/
RUN apt update && apt install -y gdb curl vim
COPY bin/fallback_proxy /bin
COPY conf/* /conf/
COPY migrations/* /migrations/
COPY migrations_common/* /migrations_common/
ENTRYPOINT /bin/fallback_proxy /conf/server.conf -E /conf/env --secdist=/vault/secrets/secrets.json --secdist-service=fintech-fallbcak-proxy -S *=data
EXPOSE 80 100
