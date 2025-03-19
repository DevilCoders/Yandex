mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    runlist:
        - components.web-api-base-lite
        - components.supervisor
        - components.dbaas-worker
        - components.jaeger-agent
        - components.dbaas-porto-controlplane
    deploy:
        version: 2
        api_host: deploy-api.db.yandex-team.ru
    monrun2: True
    monrun:
        orphan_resources_args: " -v porto -e 'meta.*.db.yandex.net'"
    dbaas_worker:
        config:
            deploy:
                version: 2
                url_v2: https://deploy-api.db.yandex-team.ru
                group: porto-prod
                attempts: 4
            dbm:
                url: https://mdb.yandex-team.ru/

include:
    - envs.prod
    - mdb_controlplane_porto_prod.common
    - mdb_controlplane_porto_prod.common.ui
    - porto.prod.dbaas.worker
    - porto.prod.pgusers.dbaas_worker
    - porto.prod.s3.pgaas_s3backup
    - porto.prod.selfdns.realm-mdb
    - porto.prod.dbaas.jaeger_agent
