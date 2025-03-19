mine_functions:
    grains.item:
        - id
        - ya
        - virtual

data:
    dbaas:
        vtype: compute
    runlist:
        - components.network
        - components.monrun2
        - components.mdb-cms-instance-autoduty
        - components.jaeger-agent
        - components.dbaas-compute-controlplane
        - components.yasmagent
    monrun2: True
    solomon:
        cluster: mdb_cms_instance_autoduty_compute_prod
    use_yasmagent: False
    metadb:
        addrs:
            - meta-dbaas01f.yandexcloud.net:6432
            - meta-dbaas01h.yandexcloud.net:6432
            - meta-dbaas01k.yandexcloud.net:6432
        password: {{ salt.yav.get("ver-01f2h24pmtjpz4meeq8x7gans7[password]") }}
    cmsdb:
        addrs:
            - mdb-cmsdb01-rc1a.yandexcloud.net:6432
            - mdb-cmsdb01-rc1b.yandexcloud.net:6432
            - mdb-cmsdb01-rc1c.yandexcloud.net:6432
        password: {{ salt.yav.get("ver-01f2h24pmtjpz4meeq8x7gans7[password]") }}
    mdb_cms:
        service_account:
            id: "yc.mdb.cms"
            key_id: {{ salt.yav.get("ver-01f7bc8d55c0yd0cfkehr1270m[key_id]") }}
            private_key: {{ salt.yav.get("ver-01f7bc8d55c0yd0cfkehr1270m[private_key]") | yaml_encode }}
        max_concurrent_tasks: 40
        is_compute: True
    juggler:
        oauth: {{ salt.yav.get('ver-01ef92vh0qyjk917xxk50591bs[juggler_oauth]') }} # TODO
    fqdn_suffixes:
        controlplane: "yandexcloud.net"
        unmanaged_dataplane: "mdb.yandexcloud.net"
        managed_dataplane: "db.yandex.net"

include:
    - envs.compute-prod
    - mdb_controlplane_compute_prod.common
    - mdb_controlplane_compute_prod.common.solomon
    - mdb_controlplane_compute_prod.common.deploy
    - mdb_controlplane_compute_prod.common.mlock
    - mdb_controlplane_compute_prod.common.health
    - mdb_controlplane_compute_prod.common.mdb_metrics
    - mdb_controlplane_compute_prod.common.enabled_mw
    - compute.prod.jaeger_agent
