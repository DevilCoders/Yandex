include:
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - compute.prod.solomon

data:
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.yandexcloud.net
    cauth_use: False
    ipv6selfdns: True
    second_selfdns: True
    runlist:
        - components.zk
    zk:
        version: '3.5.5-1+yandex19-3067ff6'
        jvm_xmx: '512M'
        config:
            dataDir: /data
            snapCount: 1000000
            fsync.warningthresholdms: 500
            maxSessionTimeout: 60000
            autopurge.purgeInterval: 1  # Purge hourly
            reconfigEnabled: 'false'
    monrun2: True
    mdb_metrics:
        enabled: True
        system_sensors: True
        main:
            yasm_tags_cmd: '/usr/local/yasmagent/mdb_zk_getter.py'
    solomon:
        agent: https://solomon.cloud.yandex-team.ru/push/json
        push_url: https://solomon.cloud.yandex-team.ru/api/v2/push
        project: yandexcloud
        cluster: 'mdb_{ctype}'
        service: yandexcloud_dbaas
    dbaas:
        vtype: compute
    dbaas_compute:
        vdb_setup: False

firewall:
    policy: ACCEPT
    REJECT:
      - net: ::/0
        type: addr6
        ports:
          # Quorum port
          - '2888'
          # Cluster intercommunication port.
          - '3888'
      - net: 0/0
        type: addr4
        ports:
          - '2888'
          - '3888'
