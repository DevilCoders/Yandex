{% from "mdb_controlplane_israel/map.jinja" import generated with context %}

# Explictly include mdb-metrics, cause nginx depends on it https://paste.yandex-team.ru/9471928
# In another installtions we use YAV with cache in local redis, so get mdb-metrics include with redis component
data:
    runlist:
        - components.deploy.salt-master
        - components.deploy.agent
        - components.dbaas-compute-controlplane
        - components.monrun2.http-ping
        - components.monrun2.http-tls
        - components.mdb-metrics
    monrun:
        http_url: 'https://localhost'
    system:
        journald:
            system_max_use: '10G'
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py
    salt_master:
        worker_threads: {{ 256 // 32 * generated.clusters.deploy_salt.resources.cores }}
        sock_pool_size: 32
        log_level: debug
        use_prometheus_metrics: true
        s3_images_bucket: {{ generated.salt_images_bucket }}
        root: /salt-master-srv
    s3:
        host: storage-internal.cloudil.com
        access_key_id: YCF08eUlDUKJ75b0V1vgl-pxY
        access_secret_key: {{ salt.lockbox.get("bcnpkemk6n82816uboc7").secret | tojson }}
    iam_ts: 'ts.private-api.yandexcloud.co.il:14282'


include:
    - envs.compute-prod
    - mdb_controlplane_israel.common:
        defaults:
            salt_version: '3002.7+ds-1+yandex0'
    - mdb_controlplane_israel.common.cert
    - mdb_controlplane_israel.deploy_salt.ext_pillars
    - mdb_controlplane_israel.deploy_salt.keys_v2
    - mdb_controlplane_israel.deploy_salt.mdb_deploy_salt
