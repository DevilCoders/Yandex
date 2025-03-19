data:
    token_service:
        address: ts.private-api.cloud-preprod.yandex.net:4282
    solomon:
        ca_path: /opt/yandex/allCAs.pem
        agent: https://solomon.cloud.yandex-team.ru/push/json
        push_url: https://solomon.cloud.yandex-team.ru/api/v2/push
        project: yandexcloud
        cluster: 'mdb_{ctype}'
        service: yandexcloud_dbaas
        sa_id: {{ salt.yav.get('ver-01ep4q27cj2a6dx9xm03wf1dgc[service_account_id]') }}
        sa_private_key: {{ salt.yav.get('ver-01ep4q27cj2a6dx9xm03wf1dgc[private_key]') | yaml_dquote }}
        sa_key_id: {{ salt.yav.get('ver-01ep4q27cj2a6dx9xm03wf1dgc[key_id]') }}
