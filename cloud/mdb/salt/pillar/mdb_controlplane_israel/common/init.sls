{% from "mdb_controlplane_israel/map.jinja" import generated with context %}
data:
    ntp:
        servers:
            - ntp1.infra.yandexcloud.co.il
            - ntp2.infra.yandexcloud.co.il
    deploy:
        version: 3
        api_host: deploy.mdb-cp.yandexcloud.co.il
        agent:
            s3:
                host: storage-internal.cloudil.com
                use_yc_metadata_token: True
                anonymous: False
            bucket: {{ generated.salt_images_bucket }}
    salt_masterless: True
    dist:
        bionic:
            secure: True
        pgdg:
            absent: True
    cauth_use: False
    ipv6selfdns: False
    selfdns_disable: True
    salt_version: {{ salt_version | default('3001.7+ds-1+yandex0') }}
    salt_py_version: 3
    monrun2: True
    mdb_metrics:
        enabled: True
    use_yasmagent: False
    dbaas:
        vtype: compute

