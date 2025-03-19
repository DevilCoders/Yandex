{% from "mdb_controlplane_israel/map.jinja" import generated as israel_generated with context %}

data:
    runlist:
        - components.deploy.salt-sync
        - components.network
        - components.dbaas-porto-controlplane
    monrun2: True
    use_yasmagent: False
    deploy:
        version: 2
        api_host: deploy-api.db.yandex-team.ru
    network:
        eth0-mtu: 1450
    mdb-salt-sync:
        sentry:
            dsn: {{ salt.yav.get('ver-01ecan1ft2kseny3s5m0fdxkan[dsn]') }}
        publish:
            envs:
                - bucket: mdb-salt-images-test # porto-test
                  s3:
                      host: s3.mds.yandex.net
                      region: ru-central-1
                      access_key: "{{ salt.yav.get('ver-01dvwygzaad8wghwa5favcvk0a[key]') }}"
                      secret_key: "{{ salt.yav.get('ver-01dvwygzaad8wghwa5favcvk0a[secret]') }}"
                - bucket: mdb-salt-images  # compute-preprod
                  s3:
                      host: storage.cloud-preprod.yandex.net
                      region: ru-central-1
                      access_key: {{ salt.yav.get('ver-01ede0sga4dt13t2t3krpexyzj[access_key]') }}
                      secret_key: {{ salt.yav.get('ver-01ede0sga4dt13t2t3krpexyzj[secret_key]') }}
                - bucket: mdb-salt-images  # porto-prod
                  s3:
                      host: s3.mds.yandex.net
                      region: ru-central-1
                      access_key: {{ salt.yav.get('ver-01fvtqb12bqr6r8brn8g369p8f[id]') }}
                      secret_key: {{ salt.yav.get('ver-01fvtqb12bqr6r8brn8g369p8f[secret]') }}
                - bucket: mdb-salt-images  # compute-prod
                  s3:
                      host: storage.yandexcloud.net
                      region: ru-central-1
                      access_key: {{ salt.yav.get('ver-01edefgtgxxs0a2e3avm6ntxhm[access_key]') }}
                      secret_key: {{ salt.yav.get('ver-01edefgtgxxs0a2e3avm6ntxhm[secret_key]') }}
                - bucket: mdb-salt-images  # datacloud
                  possibly_unavailable: True
                  s3:
                      host: s3.dualstack.eu-central-1.amazonaws.com
                      region: eu-central-1
                      access_key: {{ salt.yav.get('ver-01f105q4j81rqqhhfp2f9wv327[access_key]') }}
                      secret_key: {{ salt.yav.get('ver-01f105q4j81rqqhhfp2f9wv327[secret_key]') }}
                - bucket: {{ israel_generated.salt_images_bucket }}
                  possibly_unavailable: True
                  s3:
                      host: storage.cloudil.com
                      region: il1
                      access_key: {{ salt.yav.get('ver-01g15mx2w3jws7qtqq2nke9tre[access_key]') }}
                      secret_key: {{ salt.yav.get('ver-01g15mx2w3jws7qtqq2nke9tre[secret_key]') }}
    zk:
        hosts:
            - zkeeper01e.db.yandex.net:2181
            - zkeeper01f.db.yandex.net:2181
            - zkeeper01h.db.yandex.net:2181
        id: mdb-salt-sync

include:
    - envs.qa
    - mdb_controlplane_porto_prod.common
    - porto.prod.dbaas.robot-pgaas-deploy_ssh_key
    - porto.prod.s3.pgaas_s3backup
