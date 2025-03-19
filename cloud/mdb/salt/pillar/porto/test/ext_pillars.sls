data:
    dbaas_pillar:
        urls:
            - 'https://internal-api.db.yandex-team.ru/api/v1.0/config/'
            - 'https://internal-api-test.db.yandex-team.ru/api/v1.0/config/'
        access_id: {{ salt.yav.get('ver-01dtva25pf054ks1cgyg62z73y[id]') }}
        access_secret: {{ salt.yav.get('ver-01dtva25pf054ks1cgyg62z73y[secret]') }}
        api_pub_key: {{ salt.yav.get('ver-01dtva8kd1rv36v88s3j248102[public]') }}
        salt_sec_key: {{ salt.yav.get('ver-01dtva8kd1rv36v88s3j248102[private]') }}
    mdb_secrets:
        salt_private_key: {{ salt.yav.get('ver-01dtvcq2g67tyw5c6n0a0b5y0s[private]') }}
        public_key: {{ salt.yav.get('ver-01dtvcq2g67tyw5c6n0a0b5y0s[public]') }}
        oauth: {{ salt.yav.get('ver-01dtvcq2g67tyw5c6n0a0b5y0s[oauth]') }}
        cert_url: 'https://mdb-secrets-test.db.yandex.net/v1/cert'
        gpg_url: 'https://mdb-secrets-test.db.yandex.net/v1/gpg'
        use_iam_tokens: false
        sa_private_key: ''
        sa_id: ''
        sa_key_id: ''
        iam_hostname: ''
        tokens_url: ''
    dom0porto:
        configurations:
            - url: 'https://mdb.yandex-team.ru/api/v2/pillar/'
              token: {{ salt.yav.get('ver-01g36bbsj5737m2rgc94n4brsm[token]') }}
              include_only: ['*1-test-*.mail.yandex.net', '*1-pgaas-*.db.yandex.net', '*1-*.mail.yandex.net', 'pg*.disk.yandex.net']
            - url: 'https://mdb-test.db.yandex-team.ru/api/v2/pillar/'
              token: {{ salt.yav.get('ver-01g36bbq5z2kk40ep2d66eb9c0[token]') }}
              include_only: ['*1-test-*.db.yandex.net']
