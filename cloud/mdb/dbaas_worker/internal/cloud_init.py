def cloud_init_user_data(fqdn: str, deploy_version: str, mdb_deploy_api_host: str) -> str:
    return '''## template: jinja
#cloud-config
deploy_version: '2'
mdb_deploy_api_host: {mdb_deploy_api_host}
fqdn: {fqdn}
hostname: {fqdn}
write_files:
-   path: /etc/yandex/mdb-deploy/mdb_deploy_api_host
    content: {mdb_deploy_api_host}
    permissions: '0644'
-   path: /etc/yandex/mdb-deploy/deploy_version
    content: '{deploy_version}'
    permissions: '0644'
-   path: /etc/salt/minion_id
    content: {fqdn}
    permissions: '0640'
runcmd:
-   update-rc.d -f salt-minion defaults
-   rm -f /root/.ssh/authorized_keys
-   service mdb-ping-salt-master restart
'''.format(
        fqdn=fqdn,
        mdb_deploy_api_host=mdb_deploy_api_host,
        deploy_version=deploy_version,
    )
