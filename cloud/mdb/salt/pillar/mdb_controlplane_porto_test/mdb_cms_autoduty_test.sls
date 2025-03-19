mine_functions:
    grains.item:
        - id
        - ya
        - virtual

data:
    runlist:
        - components.network
        - components.monrun2
        - components.mdb-cms-instance-autoduty
        - components.jaeger-agent
        - components.dbaas-porto-controlplane
        - components.yasmagent
    monrun2: True
    solomon:
        cluster: mdb_cms_instance_autoduty_porto_test
    use_yasmagent: False
    metadb:
        addrs:
            - meta-test01f.db.yandex.net:6432
            - meta-test01h.db.yandex.net:6432
            - meta-test01k.db.yandex.net:6432
        password: {{ salt.yav.get("ver-01e43k7em77zrmx608bmzjr1xz[ password ]") }}
    cmsdb:
        addrs:
            - cms-db-test01f.db.yandex.net:6432
            - cms-db-test01h.db.yandex.net:6432
            - cms-db-test01k.db.yandex.net:6432
        password: {{ salt.yav.get('ver-01e43k7em77zrmx608bmzjr1xz[ password ]') }}
    mdb_cms:
        service_account:
            id: {{ salt.yav.get("ver-01erxxkr406cxbrdff2crebbxg[service_account_id]") }}
            key_id: {{ salt.yav.get("ver-01erxxkr406cxbrdff2crebbxg[key_id]") }}
            private_key: {{ salt.yav.get("ver-01erxxkr406cxbrdff2crebbxg[private_key]") | yaml_encode }}
    juggler:
        oauth: {{ salt.yav.get('ver-01ef92vh0qyjk917xxk50591bs[juggler_oauth]') }}
    deploy:
        token: {{ salt.yav.get('ver-01ef92vh0qyjk917xxk50591bs[deploy_oauth]') }}

include:
    - envs.dev
    - mdb_controlplane_porto_test.common
    - mdb_controlplane_porto_test.common.selfdns
    - mdb_controlplane_porto_test.common.solomon
    - mdb_controlplane_porto_test.common.deploy
    - mdb_controlplane_porto_test.common.mlock
    - mdb_controlplane_porto_test.common.health
    - mdb_controlplane_porto_test.common.mdb_metrics
    - porto.test.dbaas.jaeger_agent
