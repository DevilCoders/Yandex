ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

RUN mkdir -p /opt/yandex/dbaas-worker/
RUN ln -s  /opt/yandex/dbaas-worker/dbaas-worker /opt/yandex/dbaas-worker/cert-host
RUN ln -s  /opt/yandex/dbaas-worker/dbaas-worker /opt/yandex/dbaas-worker/conductor-migrate
RUN ln -s  /opt/yandex/dbaas-worker/dbaas-worker /opt/yandex/dbaas-worker/create-service-accounts
RUN ln -s  /opt/yandex/dbaas-worker/dbaas-worker /opt/yandex/dbaas-worker/fix-ext-dns
RUN ln -s  /opt/yandex/dbaas-worker/dbaas-worker /opt/yandex/dbaas-worker/fix-service-account
RUN ln -s  /opt/yandex/dbaas-worker/dbaas-worker /opt/yandex/dbaas-worker/force-resource-preset-change
RUN ln -s  /opt/yandex/dbaas-worker/dbaas-worker /opt/yandex/dbaas-worker/free-dom0
RUN ln -s  /opt/yandex/dbaas-worker/dbaas-worker /opt/yandex/dbaas-worker/replace-rootfs
RUN ln -s  /opt/yandex/dbaas-worker/dbaas-worker /opt/yandex/dbaas-worker/resetup-host
RUN ln -s  /opt/yandex/dbaas-worker/dbaas-worker /opt/yandex/dbaas-worker/task-run
RUN ln -s  /opt/yandex/dbaas-worker/dbaas-worker /opt/yandex/dbaas-worker/update-sgroups
RUN ln -s  /opt/yandex/dbaas-worker/dbaas-worker /opt/yandex/dbaas-worker/worker-shell
COPY build/dbaas-worker /opt/yandex/dbaas-worker/
RUN useradd -ms /bin/bash dbaas-worker

USER dbaas-worker
WORKDIR /etc/yandex/dbaas-worker
CMD ["/opt/yandex/dbaas-worker/dbaas-worker", "-c", "/etc/yandex/dbaas-worker/dbaas-worker.conf", "/etc/yandex/dbaas-worker-secrets/dbaas-worker-secrets.conf"]
