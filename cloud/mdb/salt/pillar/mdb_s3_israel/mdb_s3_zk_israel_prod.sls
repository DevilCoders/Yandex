include:
    - compute_vtype
    - envs.compute-prod
    - mdb_s3_israel.common

data:
    runlist:
        - components.zk
        - components.linux-kernel
        - components.deploy.agent
    dbaas_compute:
        vdb_setup: False
    zk:
        version: '3.5.5-1+yandex19-3067ff6'
        jvm_xmx: '4G'
        nodes:
            zk01-01-il1-a.mdb-s3.yandexcloud.co.il: 1
            zk01-02-il1-a.mdb-s3.yandexcloud.co.il: 2
            zk01-03-il1-a.mdb-s3.yandexcloud.co.il: 3
        config:
            snapCount: 1000000
            fsync.warningthresholdms: 500
            maxSessionTimeout: 60000
            autopurge.purgeInterval: 1  # Purge hourly
            reconfigEnabled: 'false'

firewall:
    policy: ACCEPT
    ACCEPT:
      - net: 2a11:f740:1:0:9000:1d::/96
        type: addr6
        ports:
          # Quorum port
          - '2888'
          # Cluster intercommunication port.
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
