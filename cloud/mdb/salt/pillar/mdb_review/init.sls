mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

data:
    runlist:
        - components.gerrit
        - components.monrun2.http-tls
    l3host: True
    ipv6selfdns: True
    monrun2: True
    pg_ssl_balancer: review.db.yandex-team.ru
    use_mdbsecrets: True
    blackbox-sso:
        host: review.db.yandex-team.ru
    gerrit_backup:
        key_id: {{ salt.yav.get('ver-01g7ey2ramqt8r3k1xhn3swfbx[key_id]') }}
        secret_key: {{ salt.yav.get('ver-01g7ey2ramqt8r3k1xhn3swfbx[secret_key]') }}
        passphrase: {{ salt.yav.get('ver-01g7ey2ramqt8r3k1xhn3swfbx[passphrase]') }}

include:
    - envs.dev
    - porto.prod.selfdns.realm-mdb
    - porto.prod.ci.gerrit
