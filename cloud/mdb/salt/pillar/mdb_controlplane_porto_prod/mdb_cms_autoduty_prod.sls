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
        cluster: mdb_cms_instance_autoduty_porto_prod
    use_yasmagent: False
    metadb:
        addrs:
            - meta01k.db.yandex.net:6432
            - meta01f.db.yandex.net:6432
            - meta01h.db.yandex.net:6432
        password: {{ salt.yav.get('ver-01e57r9nz9fa5f0nsp5kbr6dhe[password]') }}
    cmsdb:
        addrs:
            - cms-db-prod01f.db.yandex.net:6432
            - cms-db-prod01h.db.yandex.net:6432
            - cms-db-prod01k.db.yandex.net:6432
        password: {{ salt.yav.get('ver-01e57r9nz9fa5f0nsp5kbr6dhe[password]') }}
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
    - envs.prod
    - mdb_controlplane_porto_prod.common
    - mdb_controlplane_porto_prod.common.solomon
    - mdb_controlplane_porto_prod.common.deploy
    - mdb_controlplane_porto_prod.common.mlock
    - mdb_controlplane_porto_prod.common.health
    - mdb_controlplane_porto_prod.common.mdb_metrics
    - porto.prod.dbaas.jaeger_agent
