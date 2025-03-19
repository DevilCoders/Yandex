include:
    - envs.qa
    - mdb_controlplane_compute_preprod.common
    - mdb_controlplane_compute_preprod.common.solomon

data:
    cauth_use: False
    ipv6selfdns: True
    second_selfdns: True
    runlist:
        - components.zk
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.cloud-preprod.yandex.net
    zk:
        version: '3.5.5-1+yandex19-3067ff6'
        jvm_xmx: '512M'
        nodes:
            zk-dbaas-preprod01f.cloud-preprod.yandex.net: 1
            zk-dbaas-preprod01h.cloud-preprod.yandex.net: 2
            zk-dbaas-preprod01k.cloud-preprod.yandex.net: 3
        config:
            dataDir: /data
            snapCount: 1000000
            fsync.warningthresholdms: 500
            maxSessionTimeout: 60000
            autopurge.purgeInterval: 1  # Purge hourly
            reconfigEnabled: 'false'
    monrun2: True
    dbaas:
        vtype: compute
    dbaas_compute:
        vdb_setup: False
    mdb_metrics:
        enabled: True
        system_sensors: True
        main:
            yasm_tags_cmd: '/usr/local/yasmagent/mdb_zk_getter.py'
    solomon:
        cluster: 'mdb_mdb-common-zk-compute-preprod'

firewall:
    policy: ACCEPT
    ACCEPT:
      - net: zk-dbaas-preprod01f.cloud-preprod.yandex.net
        type: fqdn
        ports:
          - '2888'
          - '3888'
      - net: zk-dbaas-preprod01h.cloud-preprod.yandex.net
        type: fqdn
        ports:
          - '2888'
          - '3888'
      - net: zk-dbaas-preprod01k.cloud-preprod.yandex.net
        type: fqdn
        ports:
          - '2888'
          - '3888'
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
