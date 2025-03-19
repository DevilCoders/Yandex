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
        cluster: mdb_cms_instance_autoduty_compute_preprod
    use_yasmagent: False
    metadb:
        addrs:
            - meta-dbaas-preprod01f.cloud-preprod.yandex.net:6432
            - meta-dbaas-preprod01h.cloud-preprod.yandex.net:6432
            - meta-dbaas-preprod01k.cloud-preprod.yandex.net:6432
        password: {{ salt.yav.get("ver-01eybc6y3z0m5rm999b2zyc1wh[password]") }}
    cmsdb:
        addrs:
            - mdb-cmsdb-preprod01-rc1a.cloud-preprod.yandex.net:6432
            - mdb-cmsdb-preprod01-rc1c.cloud-preprod.yandex.net:6432
            - mdb-cmsdb-preprod01-rc1b.cloud-preprod.yandex.net:6432
        password: {{ salt.yav.get("ver-01eybc6y3z0m5rm999b2zyc1wh[password]") }}
    mdb_cms:
        service_account:
            id: {{ salt.yav.get("ver-01f26w1cngendqxychxzv8886y[service_account_id]") }}
            key_id: {{ salt.yav.get("ver-01f26w1cngendqxychxzv8886y[key_id]") }}
            private_key: {{ salt.yav.get("ver-01f26w1cngendqxychxzv8886y[private_key]") | yaml_encode }}
        is_compute: True
    juggler:
        oauth: {{ salt.yav.get('ver-01ef92vh0qyjk917xxk50591bs[juggler_oauth]') }} # TODO
    fqdn_suffixes:
        controlplane: "cloud-preprod.yandex.net"
        unmanaged_dataplane: "mdb.cloud-preprod.yandex.net"
        managed_dataplane: "db.yandex.net"

include:
    - envs.qa
    - mdb_controlplane_compute_preprod.common
    - mdb_controlplane_compute_preprod.common.solomon
    - mdb_controlplane_compute_preprod.common.deploy
    - mdb_controlplane_compute_preprod.common.mlock
    - mdb_controlplane_compute_preprod.common.health
    - mdb_controlplane_compute_preprod.common.mdb_metrics
    - compute.preprod.jaeger_agent
