FROM dbaas-infra-tests-base
LABEL image-cleanup=true

COPY ./config/allCAs.pem /opt/yandex/mdb-infra-test-allCAs.pem
COPY ./config/allCAs.pem /opt/yandex/allCAs.pem

# mdb-backup-worker
COPY ./bin/worker /mdb-backup-worker
COPY ./config/mdb-backup-worker.supervisor.conf /etc/supervisor/conf.d/mdb-backup-worker.conf
COPY ./config/mdb-backup-worker.yaml /worker.yaml

# mdb-backup-cli
COPY ./bin/cli /mdb-backup-cli
COPY ./config/mdb-backup-cli.yaml /cli.yaml


CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/supervisord.conf"]
