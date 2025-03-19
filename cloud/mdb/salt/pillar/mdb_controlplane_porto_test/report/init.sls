mine_functions:
    grains.item:
        - id
        - ya
        - virtual

data:
    runlist:
        - components.monrun2
        - components.yasmagent
        - components.supervisor
        - components.mdb-search-producer
        - components.katan
        - components.mdb-maintenance
        - components.dbaas-porto-controlplane
        - components.downtimer
    deploy:
        version: 2
        api_host: deploy-api-test.db.yandex-team.ru
    monrun2: True

    # KatanDB hosts
    katandb:
        hosts:
            - katan-db-test01f.db.yandex.net:6432
            - katan-db-test01h.db.yandex.net:6432
            - katan-db-test01k.db.yandex.net:6432

    # TVM Daemon config
    tvmtool:
        port: 5001
        config:
            clients:
                search-producer:
                    tvm_id: 2015044
                    dsts:
                        logbroker:
                            dst_id: 2001059

    mdb-search-producer:
        logbroker:
            endpoint: 'logbroker.yandex.net'
            topic: '/mdb/porto/test/search-entities'
        sentry:
            dsn: {{ salt.yav.get('ver-01e7qgbcmrgpykq4fqcsezrdcx[dsn]') }}
        instrumentation:
            port: 6060

    katan:
        service_account:
            id: yc.mdb.katan
            key_id: {{ salt.yav.get('ver-01ewb4vpj4ref9f30ew55gymwc[key_id]') }}
            private_key: {{ salt.yav.get('ver-01ewb4vpj4ref9f30ew55gymwc[private_key]') | yaml_encode }}
        sentry:
            dsn:
                katan: {{ salt.yav.get('ver-01e61183kqhgbcv3a5ex47z9n1[dsn]') }}
                imp: {{ salt.yav.get('ver-01e612gh4cchavzesy59dfr39k[dsn]') }}
                scheduler: {{ salt.yav.get('ver-01e612thpjmh82tfj8ess5b6ym[dsn]') }}
        allowed_deploy_groups:
            - porto-test
            - default
        instrumentation:
            port: 9090
        monrun:
            broken-schedules:
                namespaces:
                    - core
                    - postgresql
                    - mysql
                    - clickhouse
                    - redis
                    - mongodb
                    - elasticsearch
                    - kafka

    mdb-health:
        host: mdb-health-test.db.yandex.net

    yasmagent:
        instances:
            - mdbsearchproducer

    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py

    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        url: https://solomon.yandex-team.ru
        project: internal-mdb
        cluster: mdb_report_porto_test
        service: mdb
        use_name_tag: False

    mdb-maintenance:
        metadb:
            password: {{ salt.yav.get('ver-01e70b4avzh75dgax11p726fmn[password]') }}
        tasks:
            configs_dir: configs
        maintainer:
            newly_planned_limit: 1000
            newly_planned_clouds: 20
            notify_user_directly: True
            notify_user_directly_denylist:
                - foorkhlv2jt6khpv69ik
        notifier:
            endpoint: https://notify.cloud-test.yandex-team.ru
            transport:
                logging:
                    log_request_body: False
                    log_response_body: False
        ca_dir: /opt/yandex/
        service_account:
            id: foogs5ti2cuvr7ee7hsa
            key_id: {{ salt.yav.get('ver-01f35y6w3jb0284qnjkr242nwz[key_id]') }}
            private_key: {{ salt.yav.get('ver-01f35y6w3jb0284qnjkr242nwz[private_key]') | yaml_encode }}
        sentry:
            dsn: {{ salt.yav.get('ver-01fcqyb9htfrcka26fat1r5fb3[dsn]') }}
        holidays:
            enabled: True

    resource_manager:
        target: rm.cloud.yandex-team.ru:443

include:
    - envs.dev
    - mdb_controlplane_porto_test.common
    - mdb_controlplane_porto_test.common.selfdns
    - mdb_controlplane_porto_test.common.mlock
    - mdb_controlplane_porto_test.common.metadb
    - mdb_controlplane_porto_test.common.token_service
    - mdb_controlplane_porto_test.common.ui
    - mdb_controlplane_porto_test.common.zk
    - mdb_controlplane_porto_test.report.downtimer
    - porto.test.dbaas.prefixes
    - porto.test.dbaas.mdb_search_producer
    - porto.test.pgusers.mdb_search_producer
    - porto.test.pgusers.katan_imp
    - porto.test.pgusers.katan
    - porto.test.pgusers.mdb_downtimer
    - porto.test.dbaas.solomon
