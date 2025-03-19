mine_functions:
    grains.item:
        - id
        - ya

data:
    tvm:
        client_secret: {{ salt.yav.get('ver-01enskgccs24vzp0tkf2dk5v22[client_secret]') }}
        client_id: "2024459"
        env: "test"
    yav:
        token: {{ salt.yav.get('ver-01f38j6k3th7kd224j6mkpstmm[token]') }}
    runlist:
        - components.web-api-base
        - components.idm_service
    monrun2: True
    monrun:
        http_ping:
            url: http://localhost/ping
    cn: idm.test.yandex-team.ru

include:
    - envs.dev
    - porto.prod.pgusers.idm_service
    - porto.prod.dbaas.mdb_idm_service_test
    - porto.prod.selfdns.realm-mdb
