data:
    iam-token-reissuer:
        credentials-path: 'data:solomon'
    solomon:
        agent: https://solomon.cloud.yandex-team.ru/push/json
        push_url: https://solomon.cloud.yandex-team.ru/api/v2/push
        url: https://solomon.cloud.yandex-team.ru
        project: yandexcloud
        service: yandexcloud_dbaas

include:
    - compute.preprod.solomon
