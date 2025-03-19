data:
    common:
        dh: |
            {{ salt.yav.get('ver-01e95m6158s2q7cqexmzvvwcrw[private]') | indent(12) }}
    mdb-deploy-saltkeys:
        config:
            deploy_api_token: {{ salt.yav.get('ver-01e95mmepwv2wrj0vxqa9sq73p[token]') }}
            saltapi:
                user: mdb-deploy-salt-api
                password: {{ salt.yav.get('ver-01e95mg7qb867yejhqrwqy8f6r[password]') }}
                eauth: pam
    mdb-deploy-salt-api:
        salt_api_user: mdb-deploy-salt-api
        salt_api_password: {{ salt.yav.get('ver-01e95mg7qb867yejhqrwqy8f6r[hash]') }}
        tls_key: |
            {{ salt.yav.get('ver-01e95m20x4kgqpxzj4wypq598d[private]') | indent(12) }}
        tls_crt: |
            {{ salt.yav.get('ver-01e95m20x4kgqpxzj4wypq598d[cert]') | indent(12) }}
