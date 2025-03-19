data:
    common:
        dh: |
            {{ salt.yav.get('ver-01dvqdga8xhrb13qqwffch3b3r[token]') | indent(12) }}
    mdb-deploy-saltkeys:
        config:
            deploy_api_token: {{ salt.yav.get('ver-01dvqdn9z1ze4r50ggrtxxcp4n[token]') }}
            saltapi:
                user: mdb-deploy-salt-api
                password: {{ salt.yav.get('ver-01dvqg2wjt5a3tvmdmvmr462h4[password]') }}
                eauth: pam
    mdb-deploy-salt-api:
        salt_api_user: mdb-deploy-salt-api
        salt_api_password: {{ salt.yav.get('ver-01dvqg6kr43m00tht0e6yq272w[password]') }}

include:
    - porto.test.dbaas.robot-pgaas-deploy_ssh_key
