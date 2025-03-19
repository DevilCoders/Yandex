include:
    - compute_vtype
    - envs.dev
    - compute.preprod.s3.solomon
    - compute.preprod.selfdns.realm-cloud-mdb

data:
    cauth_use: False
    ipv6selfdns: True
    monrun2: True
    runlist:
        - components.zk
        - components.linux-kernel
        - components.dbaas-compute.underlay
    zk:
        version: '3.5.5-1+yandex19-3067ff6'
        jvm_xmx: '2G'
        nodes:
            zk-s3-compute01f.svc.cloud-preprod.yandex.net: 1
            zk-s3-compute01h.svc.cloud-preprod.yandex.net: 2
            zk-s3-compute01k.svc.cloud-preprod.yandex.net: 3
        config:
            snapCount: 1000000
            fsync.warningthresholdms: 500
            maxSessionTimeout: 60000
            autopurge.purgeInterval: 1  # Purge hourly
            reconfigEnabled: 'false'
    mdb_metrics:
        enabled: True
        system_sensors: True
        main:
            yasm_tags_cmd: '/usr/local/yasmagent/mdb_zk_getter.py'
    solomon:
        agent: https://solomon.cloud.yandex-team.ru/push/json
        push_url: https://solomon.cloud.yandex-team.ru/api/v2/push
        project: yandexcloud
        cluster: 'mdb_mdb-s3-zk-compute-preprod'
        service: yandexcloud_dbaas
    use_yasmagent: False

firewall:
    policy: ACCEPT
    ACCEPT:
      - net: _C_MDB_S3_ZK_COMPUTE_PREPROD_
        type: macro
        ports:
          - '2888'
          - '3888'
      - net: _C_MDB_S3_COMPUTE_PREPROD_
        type: macro
        ports:
          - '2181'
    REJECT:
      - net: '::/0'
        type: addr6
        ports:
          # Quorum port
          - '2888'
          # Cluster intercommunication port.
          - '3888'
      - net: '0/0'
        type: addr4
        ports:
          - '2888'
          - '3888'
      - net: '::/0'
        type: addr6
        ports:
          - '2181'
      - net: '0/0'
        type: addr4
        ports:
          - '2181'
