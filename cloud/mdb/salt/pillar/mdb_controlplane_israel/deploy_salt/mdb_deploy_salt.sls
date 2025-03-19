{% set cert = salt.certificate_manager.get() %}
data:
    common:
        dh: {{ salt.lockbox.get('bcntklecnhcuiguoq3ap').dhparams | tojson }}
    mdb-deploy-saltkeys:
        config:
            saltapi:
                user: mdb-deploy-salt-api
                password: {{ salt.lockbox.get('bcnso5bskmg9j9m8n2b2').password | tojson }}
                eauth: pam
    mdb-deploy-salt-api:
        salt_api_user: mdb-deploy-salt-api
        salt_api_password: {{ salt.lockbox.get('bcnso5bskmg9j9m8n2b2').hash | tojson }}
        tls_key: {{ cert['cert.key']  | tojson }}
        tls_crt: {{ cert['cert.crt'] | tojson }}
