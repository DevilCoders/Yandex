data:
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        ca_path: True
        project: internal-mdb
        cluster: mdb_{ctype}
        service: mdb

include:
    - porto.prod.dbaas.solomon

{# instead of intra_dom0 configuration,
   clusters config, don't use
   node and host #}
