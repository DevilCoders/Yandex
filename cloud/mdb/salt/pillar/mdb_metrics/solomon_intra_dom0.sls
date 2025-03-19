data:
    mdb_metrics:
        use_yasmagent: False
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        ca_path: True
        project: internal-mdb
        cluster: internal-mdb_dom0
        service: dom0
        node: '{unknown}'
        host: '{fqdn}'

include:
    - porto.prod.dbaas.solomon
