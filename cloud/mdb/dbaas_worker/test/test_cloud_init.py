from cloud.mdb.dbaas_worker.internal.cloud_init import cloud_init_user_data


def test_cloud_init_config_dumped():
    assert '''## template: jinja
#cloud-config
deploy_version: '2'
mdb_deploy_api_host: deploy.db.yandex.net
fqdn: a.db.yandex.net
hostname: a.db.yandex.net
write_files:
-   path: /etc/yandex/mdb-deploy/mdb_deploy_api_host
    content: deploy.db.yandex.net
    permissions: '0644'
-   path: /etc/yandex/mdb-deploy/deploy_version
    content: '2'
    permissions: '0644'
-   path: /etc/salt/minion_id
    content: a.db.yandex.net
    permissions: '0640'
runcmd:
-   update-rc.d -f salt-minion defaults
-   rm -f /root/.ssh/authorized_keys
-   service mdb-ping-salt-master restart
''' == cloud_init_user_data(
        fqdn='a.db.yandex.net',
        deploy_version='2',
        mdb_deploy_api_host='deploy.db.yandex.net',
    )
