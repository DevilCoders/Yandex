mine_functions:
    grains.item:
        - id
        - role
        - ya
        - virtual
include:
    - compute.prod.selfdns.realm-cloud-mdb
    - envs.compute-prod
data:
    runlist:
        - components.dbaas-wsus
        - components.dbaas-compute-windows
        - components.dbaas-compute-windows-controlplane
    mdb_role: 'wsus'
    mdb_plane: 'control-plane'
    os_clustered: false
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.yandexcloud.net
    selfdns-api:
        plugins-dir: C:/ProgramData/yandex/selfdns-client-data/plugins/
        plugins-enabled:
            - ipv6only_w.cmd
        service: 'https://selfdns-api.yandex.net'
        timeout: 10
        ttl: 300
    users:
        Administrator:
            password: {{ salt.yav.get('ver-01f23y3n2xg40z4pstd6hyr4w2[Administrator]') }}
