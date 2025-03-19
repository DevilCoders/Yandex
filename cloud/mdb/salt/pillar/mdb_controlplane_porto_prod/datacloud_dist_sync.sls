mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    runlist:
        - components.datacloud.dist-sync
    deploy:
        version: 2
        api_host: deploy-api.db.yandex-team.ru
    monrun2: True
    use_yasmagent: False
    dist-sync:
        sentry:
            dsn: {{ salt.yav.get('ver-01f3ct29ezrpkhb7h2vbynb5mv[dsn]') }}
        gpg:
            key_id: {{ salt.yav.get('ver-01f3at3mfd1q9da64dz52pqeg8[key_id]') }}
            public_key: {{ salt.yav.get('ver-01f3at3mfd1q9da64dz52pqeg8[public_key]') | yaml_encode }}
            secret_key: {{ salt.yav.get('ver-01f3at3mfd1q9da64dz52pqeg8[secret_key]') | yaml_encode }}
        s3:
            access_key: {{ salt.yav.get('ver-01f38j3k3d4gvx3xcw5z3n0zjh[access_key]') }}
            secret_key: {{ salt.yav.get('ver-01f38j3k3d4gvx3xcw5z3n0zjh[secret_key]') }}
            host: s3.dualstack.eu-central-1.amazonaws.com
            region: eu-central-1
        mirrors:
            - source: mdb-bionic-secure/stable/amd64
              bucket: mdb-bionic-stable-amd64
              architectures:
                - amd64
            - source: mdb-bionic-secure/stable/arm64
              bucket: mdb-bionic-stable-arm64
              architectures:
                - arm64
            - source: mdb-bionic-secure/stable/all
              bucket: mdb-bionic-stable-all
              architectures:
                - all
        packages:
            - 'Name (~ linux-image-.*)'
            - 'Name (~ clickhouse-.*)'
            -  apt-golang-s3
            -  catboost-model-lib
            -  ch-backup
            -  config-caching-dns
            -  config-ssh-banner
            -  config-yabs-ntp
            -  confluent-kafka
            -  dbaas-cron
            -  grpcurl
            -  jmx-prometheus-javaagent
            -  juggler-client
            -  kafka
            -  kafka-adminapi-gateway
            -  kafka-authorizer
            -  libfile-timesearch-perl
            -  libodbc1
            -  libpq5
            -  libzookeeper-mt2
            -  linux-tools
            -  mcelog
            -  mdb-ch-tools
            -  mdb-config-salt
            -  mdb-disklock
            -  mdb-kafka-agent
            -  mdb-ssh-keys
            -  mdb-telegraf
            -  monrun
            -  odbc-postgresql
            -  odbcinst
            -  odbcinst1debian2
            -  py4j
            -  python-kazoo
            -  python-msgpack
            -  python-zookeeper
            -  python3-py4j
            -  python3-contextvars
            -  python3-immutables
            -  python3-kazoo
            -  python3-zmq
            -  python36-confluent-kafka
            -  retry
            -  salt-common
            -  salt-minion
            -  tzdata
            -  unixodbc
            -  vector
            -  yamail-monrun-errata
            -  yamail-monrun-restart-required
            -  yandex-archive-keyring
            -  yandex-cauth
            -  yandex-dash2bash
            -  yandex-default-locale-en
            -  yandex-lib-autodetect-environment
            -  yandex-lockf
            -  yandex-mdb-metrics
            -  yandex-push-client
            -  yandex-search-user-monitor
            -  yandex-selfdns-client
            -  yandex-timetail
            -  yandex-yasmagent
            -  zk-flock
            -  zookeeper

include:
    - envs.qa
    - mdb_controlplane_porto_prod.common
