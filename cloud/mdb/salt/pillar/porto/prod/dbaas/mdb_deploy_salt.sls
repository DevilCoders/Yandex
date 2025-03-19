data:
    common:
        dh: |
            {{ salt.yav.get('ver-01dz8p0v4rr6dtc64na0ymgb62[private]') | indent(12) }}
    mdb-deploy-saltkeys:
        config:
            deploy_api_token: {{ salt.yav.get('ver-01dz8pb27s7tpg298ava8s8tdy[token]') }}
            saltapi:
                user: mdb-deploy-salt-api
                password: {{ salt.yav.get('ver-01dz63829xnskw386s17kjv8g8[password]') }}
                eauth: pam
            minion_pinger:
                max_parallel_pings: 5
    mdb-deploy-salt-api:
        salt_api_user: mdb-deploy-salt-api
        salt_api_password: {{ salt.yav.get('ver-01dz8pf9dgq3php792gdbgzre4[password]') }}

include:
    - porto.prod.dbaas.robot-pgaas-deploy_ssh_key
