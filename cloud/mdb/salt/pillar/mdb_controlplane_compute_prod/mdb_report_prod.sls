include:
    - envs.compute-prod
    - compute.prod.solomon
    - mdb_controlplane_compute_prod.common
    - mdb_controlplane_compute_prod.common.yc_logbroker
    - mdb_controlplane_compute_prod.common.metadb
    - mdb_controlplane_compute_prod.common.mlock
    - mdb_controlplane_compute_prod.common.ui
    - compute.prod.pgusers.mdb_report
    - compute.prod.pgusers.mdb_event_producer
    - compute.prod.pgusers.mdb_search_producer
    - compute.prod.pgusers.katan
    - compute.prod.pgusers.katan_imp
    - compute.prod.prefixes
    - compute.prod.image_releaser
    - compute.prod.images

data:
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.yandexcloud.net
    cauth_use: False
    ipv6selfdns: True
    second_selfdns: True
    runlist:
        - components.zk
        - components.mdb-report
        - components.monrun2
        - components.supervisor
        - components.mdb-event-producer
        - components.mdb-search-producer
        - components.image-releaser
        - components.katan
        - components.mdb-maintenance
    dbaas:
        vtype: compute
    dbaas_cron:
        status_timeout: 235
    dbaas_compute:
        vdb_setup: False
    zk:
        version: '3.5.5-1+yandex19-3067ff6'
        jvm_xmx: '512M'
        nodes:
            mdb-report01f.yandexcloud.net: 1
            mdb-report01h.yandexcloud.net: 2
            mdb-report01k.yandexcloud.net: 3
        config:
            dataDir: /data
            snapCount: 1000000
            fsync.warningthresholdms: 500
            maxSessionTimeout: 60000
            autopurge.purgeInterval: 1  # Purge hourly
            reconfigEnabled: 'false'
        hosts:
            - zk-dbaas01f.yandexcloud.net:2181
            - zk-dbaas01h.yandexcloud.net:2181
            - zk-dbaas01k.yandexcloud.net:2181

    logship:
        enabled: false

    mdb_report:
        zk_hosts: mdb-report01f.yandexcloud.net:2181,mdb-report01h.yandexcloud.net:2181,mdb-report01k.yandexcloud.net:2181
        metadb_hosts: meta-dbaas01f.yandexcloud.net,meta-dbaas01h.yandexcloud.net,meta-dbaas01k.yandexcloud.net
        offline_billing:
            topic: /yc-mdb/billing-mdb-instance
            tvm_client_id: 2002008
            tvm_secret: {{ salt.yav.get('ver-01e96a4q9vk3qf6wf99mw2v1pk[secret]') }}
            tvm_server_id: 2001059

    monrun2: True
    mdb_metrics:
        main:
            yasm_tags_cmd: '/usr/local/yasmagent/mdb_zk_getter.py'

    katandb:
        hosts:
            - mdb-katandb01-rc1a.yandexcloud.net:6432
            - mdb-katandb01-rc1b.yandexcloud.net:6432
            - mdb-katandb01-rc1c.yandexcloud.net:6432

    mdb-health:
        host: mdb-health.private-api.cloud.yandex.net

    katan:
        service_account:
            id: yc.mdb.katan
            key_id: {{ salt.yav.get('ver-01ewb5ems7hg7k3whksf2th2z3[key_id]') }}
            private_key: {{ salt.yav.get('ver-01ewb5ems7hg7k3whksf2th2z3[private_key]') | yaml_encode }}
        sentry:
            dsn:
                katan: {{ salt.yav.get('ver-01ejv4th8tggmekxjmn8p4ryx2[dsn]') }}
                imp: {{ salt.yav.get('ver-01ejv4m671h3we5vpy5bnfsdwh[dsn]') }}
                scheduler: {{ salt.yav.get('ver-01ejv4q3a7g548aheznt59pjg1[dsn]') }}
        allowed_deploy_groups:
            - default
            - compute-prod
        instrumentation:
            port: 9090
        monrun:
            broken-schedules:
                namespaces:
                    - core
                    - postgresql
                    - mysql
                    - sqlserver
                    - clickhouse
                    - redis
                    - mongodb
                    - elasticsearch
                    - kafka

    solomon:
        agent: https://solomon.cloud.yandex-team.ru/push/json
        push_url: https://solomon.cloud.yandex-team.ru/api/v2/push
        url: https://solomon.cloud.yandex-team.ru
        project: yandexcloud
        cluster: mdb_report_compute_prod
        service: yandexcloud_dbaas
        use_name_tag: True

    mdb-maintenance:
        cms:
            addr: mdb-cmsgrpcapi.private-api.cloud.yandex.net:443
        metadb:
            password: {{ salt.yav.get('ver-01e70csxrgtgbsxtee3hy2yeag[password]') }}
        tasks:
            configs_dir: configs
        maintainer:
            newly_planned_limit: 1000
            newly_planned_clouds: 20
        notifier:
            endpoint: https://notify.cloud.yandex.net
            transport:
                logging:
                    log_request_body: False
                    log_response_body: False
        ca_dir: /opt/yandex/
        service_account:
            id: {{ salt.yav.get('ver-01eq142yach27d31bshmqbrsx2[id]') }}
            key_id: {{ salt.yav.get('ver-01eq142yach27d31bshmqbrsx2[key_id]') }}
            private_key: {{ salt.yav.get('ver-01eq142yach27d31bshmqbrsx2[private_key]') | yaml_encode }}
        sentry:
            dsn: {{ salt.yav.get('ver-01fcqyb9htfrcka26fat1r5fb3[dsn]') }}
        holidays:
            enabled: True

    mdb-search-producer:
        logbroker:
            use_yc_lb: True
            topic: 'yc.search/search-data-services'
        sentry:
            dsn: {{ salt.yav.get('ver-01e95yv13pza2tw6qtpxwwzw6x[dsn]') }}
        instrumentation:
            port: 6060
        service_account:
            id: yc.mdb.search-producer
            key_id: {{ salt.yav.get('ver-01fx4xn8eqgb8psksjkvsk3sa6[key_id]') }}
            private_key: {{ salt.yav.get('ver-01fx4xn8eqgb8psksjkvsk3sa6[private_key]') | yaml_encode }}

    mdb-event-producer:
        logbroker:
            topic: 'b1gm277magvimb2gv0ld/yc-events'
        instrumentation:
            port: 7070
        service_account:
            id: yc.mdb.event-producer
            key_id: {{ salt.yav.get('ver-01et2zg5ptpgkvzmsfhx3zrevv[key_id]') }}
            private_key: {{ salt.yav.get('ver-01et2zg5ptpgkvzmsfhx3zrevv[private_key]') | yaml_encode }}

firewall:
    policy: ACCEPT
    ACCEPT:
      - net: mdb-report01f.yandexcloud.net
        type: fqdn
        ports:
          - '2181'
          - '2888'
          - '3888'
      - net: mdb-report01h.yandexcloud.net
        type: fqdn
        ports:
          - '2181'
          - '2888'
          - '3888'
      - net: mdb-report01k.yandexcloud.net
        type: fqdn
        ports:
          - '2181'
          - '2888'
          - '3888'
    REJECT:
      - net: ::/0
        type: addr6
        ports:
          # Data port
          - '2181'
          # Quorum port
          - '2888'
          # Cluster intercommunication port.
          - '3888'
      - net: 0/0
        type: addr4
        ports:
          - '2181'
          - '2888'
          - '3888'
