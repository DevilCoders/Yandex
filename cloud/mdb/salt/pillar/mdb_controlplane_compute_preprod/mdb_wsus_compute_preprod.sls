mine_functions:
    grains.item:
        - id
        - role
        - ya
        - virtual
include:
    - compute.preprod.selfdns.realm-cloud-mdb
    - envs.dev

data:
    runlist:
        - components.dbaas-wsus
        - components.dbaas-compute-windows
        - components.dbaas-compute-windows-controlplane
    mdb_role: 'wsus'
    mdb_plane: 'control-plane'
    os_clustered: false
    dbaas:
        vtype: 'compute'
        fqdn: {{salt.grains.get('fqdn')}}
        cluster_id:
            e4ufcram9s8lcset12nt
        cluster_name:
            mdb-wsus-preprod-cl
        disk_type_id:
            network-ssd
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.cloud-preprod.yandex.net
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
