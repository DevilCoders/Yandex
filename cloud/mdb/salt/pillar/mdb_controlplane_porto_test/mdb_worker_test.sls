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
        orphan_resources_args: ' -v porto'

include:
    - envs.dev
    - mdb_controlplane_porto_test.common
    - mdb_controlplane_porto_test.common.ui
    - porto.test.dbaas.worker
    - porto.prod.pgusers.dbaas_worker
    - porto.prod.s3.pgaas_s3backup
    - porto.prod.selfdns.realm-mdb
    - porto.test.dbaas.jaeger_agent
