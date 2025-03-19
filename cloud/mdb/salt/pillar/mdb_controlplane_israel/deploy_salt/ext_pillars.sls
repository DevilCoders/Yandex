{%set salt_nacl_private_key = salt.lockbox.get('bcn87hoo70v8aig6hv9p').private %}
data:
    dbaas_pillar:
        urls:
            - 'https://mdb-rest-api.private-api.yandexcloud.co.il/api/v1.0/config/'
        # here should be 'static' access key, that we fill in metadb
        access_id: {{  salt.lockbox.get('bcnhos2s5jumjvo2ni4a').access_id | yaml_dquote }}
        access_secret: {{  salt.lockbox.get('bcnhos2s5jumjvo2ni4a').access_secret | yaml_dquote }}
        # controlplane.git/helm/stands/yc-israel-prod/stand-prod/values.yaml $global.nacl.internal_api.public
        api_pub_key: 'BAYhpIZXvKlrwvwU_5OQ2IFSNKtdWgvVaGHcUdzo7DA='
        salt_sec_key: {{ salt_nacl_private_key | tojson }}
    mdb_secrets:
        # controlplane.git/helm/stands/yc-israel-prod/stand-prod/values.yaml $global.nacl.secrets-api.public
        public_key: '5eTZcq4xWlTgcAQSieQeuSiYm1EZgwfl7WEdMhN0J1g='
        salt_private_key: {{ salt_nacl_private_key | tojson }}
        use_iam_tokens: true
        sa_id: 'yc.mdb.salt-master'
        sa_private_key: {{  salt.lockbox.get('bcn9bt67m9jqtc7229qp').private_key | tojson }}
        sa_key_id: f08ra8g5k7dm63a28nma
        iam_hostname: 'iam.api.cloudil.com'
        tokens_url: 'https://iam.api.cloudil.com/iam/v1/tokens'
        cert_url: 'https://secrets.mdb-cp.yandexcloud.co.il/v1/cert'
        gpg_url: 'https://secrets.mdb-cp.yandexcloud.co.il/v1/gpg'
