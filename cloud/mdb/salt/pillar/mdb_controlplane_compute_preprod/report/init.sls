include:
    - envs.qa
    - mdb_controlplane_compute_preprod.common
    - mdb_controlplane_compute_preprod.common.solomon
    - mdb_controlplane_compute_preprod.common.yc_logbroker
    - mdb_controlplane_compute_preprod.common.metadb
    - mdb_controlplane_compute_preprod.common.mlock
    - mdb_controlplane_compute_preprod.common.ui
    - mdb_controlplane_compute_preprod.common.iam
    - mdb_controlplane_compute_preprod.report.image_releaser
    - mdb_controlplane_compute_preprod.report.downtimer
    - compute.preprod.pgusers.mdb_report
    - compute.preprod.pgusers.mdb_search_producer
    - compute.preprod.pgusers.mdb_event_producer
    - compute.preprod.pgusers.mdb_downtimer
    - compute.preprod.pgusers.katan
    - compute.preprod.pgusers.katan_imp
    - compute.preprod.prefixes
    - compute.preprod.mdb_search_producer

data:
    cauth_use: False
    ipv6selfdns: True
    second_selfdns: True
    runlist:
        - components.zk
        - components.mdb-report
        - components.monrun2
        - components.yasmagent
        - components.supervisor
        - components.mdb-search-producer
        - components.mdb-event-producer
        - components.katan
        - components.mdb-maintenance
        - components.image-releaser
        - components.downtimer
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.cloud-preprod.yandex.net
    zk:
        version: '3.5.5-1+yandex19-3067ff6'
        jvm_xmx: '512M'
        nodes:
            mdb-report-preprod01f.cloud-preprod.yandex.net: 1
            mdb-report-preprod01h.cloud-preprod.yandex.net: 2
            mdb-report-preprod01k.cloud-preprod.yandex.net: 3
        config:
            dataDir: /data
            snapCount: 1000000
            fsync.warningthresholdms: 500
            maxSessionTimeout: 60000
            autopurge.purgeInterval: 1  # Purge hourly
            reconfigEnabled: 'false'
        hosts:
            - zk-dbaas-preprod01f.cloud-preprod.yandex.net:2181
            - zk-dbaas-preprod01h.cloud-preprod.yandex.net:2181
            - zk-dbaas-preprod01k.cloud-preprod.yandex.net:2181
    mdb_report:
        zk_hosts: mdb-report-preprod01f.cloud-preprod.yandex.net:2181,mdb-report-preprod01h.cloud-preprod.yandex.net:2181,mdb-report-preprod01k.cloud-preprod.yandex.net:2181
        metadb_hosts: meta-dbaas-preprod01f.cloud-preprod.yandex.net,meta-dbaas-preprod01h.cloud-preprod.yandex.net,meta-dbaas-preprod01k.cloud-preprod.yandex.net
        offline_billing:
            use_cloud_logbroker: True
            topic: /yc.billing.service-cloud/billing-mdb-instance
            service_account:
                id: {{ salt.yav.get('ver-01ex2br7bfpd72fygamnnvc84w[key_id]') }}
                private_key: {{ salt.yav.get('ver-01ex2br7bfpd72fygamnnvc84w[private_key]') | yaml_encode }}
                public_key: {{ salt.yav.get('ver-01ex2br7bfpd72fygamnnvc84w[public_key]') | yaml_encode }}
                service_account_id: yc.mdb.billing

    logship:
        enabled: False

    katandb:
        hosts:
            - katan-db-preprod01f.cloud-preprod.yandex.net:6432
            - katan-db-preprod01h.cloud-preprod.yandex.net:6432
            - katan-db-preprod01k.cloud-preprod.yandex.net:6432

    mdb-search-producer:
        logbroker:
            use_yc_lb: True
            topic: 'yc.search/search-data-services'
        sentry:
            dsn: {{ salt.yav.get('ver-01e7qggp381d91svmqg6y87a5m[dsn]') }}
        instrumentation:
            port: 6060
        service_account:
            id: yc.mdb.search-producer
            key_id: {{ salt.yav.get('ver-01fx339jr77xc4hjw7g3ehvn9z[key_id]') }}
            private_key: {{ salt.yav.get('ver-01fx339jr77xc4hjw7g3ehvn9z[private_key]') | yaml_encode }}

    mdb-event-producer:
        logbroker:
            topic: 'aoedsmvctptsmbkj4g90/yc-events'
        instrumentation:
            port: 7070
        service_account:
            id: yc.mdb.event-producer
            key_id: {{ salt.yav.get('ver-01esrmpfv9sp0brtgc55mdbb72[key_id]') }}
            private_key: {{ salt.yav.get('ver-01esrmpfv9sp0brtgc55mdbb72[private_key]') | yaml_encode }}

    katan:
        service_account:
            id: yc.mdb.katan
            key_id: {{ salt.yav.get('ver-01ewb57wywvgfb4vd7tyv74fae[key_id]') }}
            private_key: {{ salt.yav.get('ver-01ewb57wywvgfb4vd7tyv74fae[private_key]') | yaml_encode }}
        sentry:
            dsn:
                katan: {{ salt.yav.get('ver-01e6kr8r2r9c5v3z9kaf6h1n59[dsn]') }}
                imp: {{ salt.yav.get('ver-01e6ks8afmyj25hek1rvnypvhe[dsn]') }}
                scheduler: {{ salt.yav.get('ver-01e6ks5cs8y56dq7dcnt1zhthm[dsn]') }}
        allowed_deploy_groups:
            - default
            - compute-preprod
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

    mdb-health:
        host: mdb-health.private-api.cloud-preprod.yandex.net

    solomon:
        cluster: mdb_report_compute_preprod
        use_name_tag: True

    mdb-maintenance:
        cms:
          addr: mdb-cmsgrpcapi.private-api.cloud-preprod.yandex.net:443
        tasks:
            configs_dir: configs
        metadb:
            password: {{ salt.yav.get('ver-01e70cpf4hsm9qe6n4xwt8frq6[password]') }}
        maintainer:
            newly_planned_limit: 1000
            newly_planned_clouds: 20
        notifier:
            endpoint: https://notify.cloud-preprod.yandex.net
            transport:
                logging:
                    log_request_body: True
                    log_response_body: True
        ca_dir: /opt/yandex/
        service_account:
            id: {{ salt.yav.get('ver-01eq0y0x2k5vqw6690rz3trv8m[id]') }}
            key_id: {{ salt.yav.get('ver-01eq0y0x2k5vqw6690rz3trv8m[key_id]') }}
            private_key: {{ salt.yav.get('ver-01eq0y0x2k5vqw6690rz3trv8m[private_key]') | yaml_encode }}
        sentry:
            dsn: {{ salt.yav.get('ver-01fcqyb9htfrcka26fat1r5fb3[dsn]') }}
        holidays:
            enabled: True
        disable-maintenance-sync: True

    monrun2: True
    dbaas:
        vtype: compute
    dbaas_cron:
        status_timeout: 235
    dbaas_compute:
        vdb_setup: False
    mdb_metrics:
        main:
            yasm_tags_cmd: '/usr/local/yasmagent/mdb_zk_getter.py'
    yasmagent:
        instances:
            - mdbsearchproducer

firewall:
    policy: ACCEPT
    ACCEPT:
      - net: mdb-report-preprod01f.cloud-preprod.yandex.net
        type: fqdn
        ports:
          - '2181'
          - '2888'
          - '3888'
      - net: mdb-report-preprod01h.cloud-preprod.yandex.net
        type: fqdn
        ports:
          - '2181'
          - '2888'
          - '3888'
      - net: mdb-report-preprod01k.cloud-preprod.yandex.net
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
