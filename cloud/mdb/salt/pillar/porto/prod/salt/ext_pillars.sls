data:
    dbaas_pillar:
        urls:
            - 'https://internal-api.db.yandex-team.ru/api/v1.0/config/'
            - 'https://internal-api-test.db.yandex-team.ru/api/v1.0/config/'
        access_id: '34019e42-4c7d-4208-8bde-37cc0430ca3e'
        access_secret: {{ salt.yav.get('ver-01dz8meswjsb761x7yt1qjwg66[secret]') }}
        api_pub_key: {{ salt.yav.get('ver-01dz8n3wnagrqy9n9hjzbrqhnz[public]') }}
        salt_sec_key: {{ salt.yav.get('ver-01dz8n3wnagrqy9n9hjzbrqhnz[secret]') }}
    mdb_secrets:
        salt_private_key: {{ salt.yav.get('ver-01dz8nm5w4p4fqfxbczsqcc78k[private]') }}
        public_key: {{ salt.yav.get('ver-01dz8nm5w4p4fqfxbczsqcc78k[public]') }}
        use_iam_tokens: false
        sa_private_key: ''
        sa_id: ''
        sa_key_id: ''
        iam_hostname: ''
        tokens_url: ''
        oauth: {{ salt.yav.get('ver-01dz8nm5w4p4fqfxbczsqcc78k[oauth]') }}
        cert_url: 'https://mdb-secrets.db.yandex.net/v1/cert'
        gpg_url: 'https://mdb-secrets.db.yandex.net/v1/gpg'
    dom0porto:
        configurations:
            - url: 'https://mdb.yandex-team.ru/api/v2/pillar/'
              token: {{ salt.yav.get('ver-01g36bbjbgmn2v31gqk3c8gkrg[token]') }}
              include_only: ['*1-test-*.mail.yandex.net', '*1-pgaas-*.db.yandex.net', '*1-*.mail.yandex.net', 'pg*.disk.yandex.net']
            - url: 'https://mdb-test.db.yandex-team.ru/api/v2/pillar/'
              token: {{ salt.yav.get('ver-01g3bw9wnz8hyt7aj742vd1bkq[token]') }}
              include_only: ['*1-test-*.db.yandex.net']
