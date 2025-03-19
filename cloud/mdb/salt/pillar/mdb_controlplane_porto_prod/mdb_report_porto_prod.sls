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
        - components.image-releaser
        - components.katan
        - components.mdb-maintenance
        - components.dbaas-porto-controlplane
    deploy:
        version: 2
        api_host: deploy-api.db.yandex-team.ru
    monrun2: True

    tvmtool:
        port: 5001
        config:
            clients:
                search-producer:
                    tvm_id: 2015046
                    dsts:
                        logbroker:
                            dst_id: 2001059

    katandb:
        hosts:
            - katan-db01f.db.yandex.net:6432
            - katan-db01h.db.yandex.net:6432
            - katan-db01k.db.yandex.net:6432

    mdb-search-producer:
        logbroker:
            endpoint: 'logbroker.yandex.net'
            topic: '/mdb/porto/prod/search-entities'
        sentry:
            dsn: {{ salt.yav.get('ver-01e7qg73d0vjxp7y4xveqcwkp0[dsn]') }}
        instrumentation:
            port: 6060

    katan:
        service_account:
            id: yc.mdb.katan
            key_id: {{ salt.yav.get('ver-01ewb4vpj4ref9f30ew55gymwc[key_id]') }}
            private_key: {{ salt.yav.get('ver-01ewb4vpj4ref9f30ew55gymwc[private_key]') | yaml_encode }}
        sentry:
            dsn:
                katan: {{ salt.yav.get('ver-01ecq42m5e68c268446b17trrm[dsn]') }}
                imp: {{ salt.yav.get('ver-01ecq4rsd1dkx61wpw2jh6sqvv[dsn]') }}
                scheduler: {{ salt.yav.get('ver-01ecq4vr0b2yyfs377jfb5yqag[dsn]') }}
        allowed_deploy_groups:
            - default
            - porto-prod
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
        host: health.db.yandex.net

    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        url: https://solomon.yandex-team.ru
        project: internal-mdb
        cluster: mdb_report_porto_prod
        service: mdb
        use_name_tag: False

    mdb-maintenance:
        tasks:
            configs_dir: configs
        metadb:
            password: {{ salt.yav.get('ver-01e70cjt8zshy7zvymdx5nm6e8[password]') }}
        maintainer:
            newly_planned_limit: 1000
            newly_planned_clouds: 20
            notify_user_directly: True
            notify_user_directly_denylist:
                - foorkhlv2jt6khpv69ik
        notifier:
            endpoint: https://notify.cloud.yandex-team.ru
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

    mdb_metrics:
        main:
            # Change in to zk_getter when we enable offline billing at that env
            yasm_tags_cmd: '/usr/local/yasmagent/default_getter.py'
    yasmagent:
        instances:
            - mdbsearchproducer

include:
    - envs.prod
    - mdb_controlplane_porto_prod.common
    - mdb_controlplane_porto_prod.common.metadb
    - mdb_controlplane_porto_prod.common.mlock
    - mdb_controlplane_porto_prod.common.token_service
    - mdb_controlplane_porto_prod.common.ui
    - mdb_controlplane_porto_prod.common.zk
    - porto.prod.dbaas.mdb_search_producer
    - porto.prod.pgusers.mdb_search_producer
    - porto.prod.pgusers.katan
    - porto.prod.pgusers.katan_imp
    - porto.prod.dbaas.prefixes
    - porto.prod.dbaas.image_releaser
    - porto.prod.dbaas.solomon
    - porto.prod.images

