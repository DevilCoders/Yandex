mine_functions:
    grains.item:
        - id
        - role
        - ya

data:
    tvm:
        client_secret: {{ salt.yav.get('ver-01entejwtq9mnchkm8snw8mvdm[client_secret]') }}
        client_id: "2024477"
        env: "prod"
    yav:
        token: {{ salt.yav.get('ver-01f38jh91bgfptbg9nzf8men3v[token]') }}
    runlist:
        - components.web-api-base
        - components.idm_service
        - components.network
    monrun2: True
    monrun:
        http_ping:
            url: http://localhost/ping
    cn: idm.yandex-team.ru
    network:
        l3_slb:
            virtual_ipv6:
                - 2a02:6b8:0:3400:0:767:0:4
    use_mdbsecrets: True
    pg_ssl_balancer: mdb-idm.db.yandex.net

include:
    - envs.prod
    - porto.prod.pgusers.idm_service
    - porto.prod.dbaas.mdb_idm_service
    - porto.prod.selfdns.realm-mdb
