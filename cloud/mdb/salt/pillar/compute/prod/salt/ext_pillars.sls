data:
    dbaas_pillar:
        urls:
            - 'https://mdb.private-api.cloud.yandex.net/api/v1.0/config/'
        access_id: '34019e42-4c7d-4208-8bde-37cc0430ca3e'
        access_secret: {{ salt.yav.get('ver-01e2g6sf1zyys5kbdc1h83kdtw[secret]') }}
        api_pub_key: {{ salt.yav.get('ver-01e95kbf75e8qv7bw69x2aswd0[public]') }}
        salt_sec_key: {{ salt.yav.get('ver-01e95kbf75e8qv7bw69x2aswd0[private]') }}
    mdb_secrets:
        salt_private_key: {{ salt.yav.get('ver-01fedqfdeschzjqzd2b9qbmgmw[salt_private_key]') }}
        public_key: {{ salt.yav.get('ver-01fedqfdeschzjqzd2b9qbmgmw[secrets_public_key]') }}
        use_iam_tokens: true
        sa_id: 'yc.mdb.salt-master'
        sa_private_key: |
            {{ salt.yav.get('ver-01fedzj1gytg9vh21q6n7kmm0s[private_key]') | indent(12) }}
        sa_key_id: 'ajejnbd0qse13d7nd9ee'
        iam_hostname: 'iam.api.cloud.yandex.net'
        tokens_url: 'https://iam.api.cloud.yandex.net/iam/v1/tokens'
        cert_url: 'https://mdb-secrets.private-api.cloud.yandex.net/v1/cert'
        gpg_url: 'https://mdb-secrets.private-api.cloud.yandex.net/v1/gpg'
