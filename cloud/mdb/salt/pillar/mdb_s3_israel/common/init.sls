data:
    ntp:
        servers:
            - ntp1.infra.yandexcloud.co.il
            - ntp2.infra.yandexcloud.co.il
    deploy:
        version: 3
        api_host: deploy.mdb-cp.yandexcloud.co.il
    cauth_use: False
    ipv6selfdns: False
    selfdns_disable: True
    salt_version: '3001.7+ds-1+yandex0'
    salt_py_version: 3
    monrun2: True
    mdb_metrics:
        enabled: True
    use_yasmagent: False
    salt_masterless: True

