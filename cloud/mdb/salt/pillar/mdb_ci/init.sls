mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    runlist:
      - components.jenkins
      - components.monrun2.http-tls
    l3host: True
    ipv6selfdns: True
    use_nat64_dns: True
    monrun2: True
    pg_ssl_balancer: jenkins.db.yandex-team.ru
    use_mdbsecrets: True
    blackbox-sso:
        host: jenkins.db.yandex-team.ru
    jenkins_backup:
        key_id: {{ salt.yav.get('ver-01g7f1tzwtpzwsdcbex4ttm6b4[key_id]') }}
        secret_key: {{ salt.yav.get('ver-01g7f1tzwtpzwsdcbex4ttm6b4[secret_key]') }}
        passphrase: {{ salt.yav.get('ver-01g7f1tzwtpzwsdcbex4ttm6b4[passphrase]') }}

include:
    - envs.dev
    - porto.prod.selfdns.realm-mdb
    - porto.prod.ci.jenkins
