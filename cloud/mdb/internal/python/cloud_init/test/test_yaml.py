from cloud.mdb.internal.python.cloud_init.config import (
    CloudConfig,
    WriteFiles,
    Users,
    LegacyKeyValue,
    Hostname,
    UpdateEtcHosts,
)
from cloud.mdb.internal.python.cloud_init.yaml import dump_cloud_config


def test_empty_config():
    assert '''## template: jinja
#cloud-config
{}
''' == dump_cloud_config(
        CloudConfig()
    )


def test_can_add_any_order():
    config = CloudConfig()
    config.append_module_item(
        WriteFiles('/etc/yandex/mdb-deploy/deploy_version', '2', '0644'),
        Users(name='worker', keys=['ecdsa-sha2-nistp256 AAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBF']),
        WriteFiles(
            '/etc/yandex/mdb-deploy/mdb_deploy_api_host', 'dataproc-chapson.infratest.db.yandex.net:8443', '0644'
        ),
        WriteFiles('/etc/salt/minion_id', 'rc1c-hqd4itl85nu1bbho.mdb.cloud-preprod.yandex.net', '0640'),
    )
    config.append_module_item(
        Users(name='chapson', keys=['ecdsa-sha2-nistp256 AAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBF']),
    )
    assert '''## template: jinja
#cloud-config
write_files:
-   path: /etc/yandex/mdb-deploy/deploy_version
    content: '2'
    permissions: '0644'
-   path: /etc/yandex/mdb-deploy/mdb_deploy_api_host
    content: dataproc-chapson.infratest.db.yandex.net:8443
    permissions: '0644'
-   path: /etc/salt/minion_id
    content: rc1c-hqd4itl85nu1bbho.mdb.cloud-preprod.yandex.net
    permissions: '0640'
users:
-   name: worker
    sudo: ALL=(ALL) NOPASSWD:ALL
    shell: /bin/bash
    ssh_authorized_keys:
    - ecdsa-sha2-nistp256 AAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBF
-   name: chapson
    sudo: ALL=(ALL) NOPASSWD:ALL
    shell: /bin/bash
    ssh_authorized_keys:
    - ecdsa-sha2-nistp256 AAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBF
''' == dump_cloud_config(
        config
    )


def test_legacy_values_first():
    config = CloudConfig()
    config.append_module_item(
        WriteFiles('/etc/yandex/mdb-deploy/deploy_version', '2', '0644'),
        LegacyKeyValue('deploy_version', '2'),
        Hostname('a.db.yandex.net'),
        UpdateEtcHosts(True),
        LegacyKeyValue('mdb_deploy_api_host', 'db.yandex.net'),
    )
    assert '''## template: jinja
#cloud-config
deploy_version: '2'
mdb_deploy_api_host: db.yandex.net
write_files:
-   path: /etc/yandex/mdb-deploy/deploy_version
    content: '2'
    permissions: '0644'
hostname: a.db.yandex.net
manage_etc_hosts: true
''' == dump_cloud_config(
        config
    )
