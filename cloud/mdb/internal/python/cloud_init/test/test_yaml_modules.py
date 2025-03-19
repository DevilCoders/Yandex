import yaml
from cloud.mdb.internal.python.cloud_init.yaml.modules import (
    LegacyKeyValue,
    WriteFiles,
    Users,
    UpdateEtcHosts,
    Hostname,
    RunCMD,
)
from cloud.mdb.internal.python.cloud_init.yaml import Dumper


def dump(data) -> str:
    return yaml.dump(data, Dumper=Dumper)


def test_write_files():
    subj = WriteFiles('/foo/bar', '1', '0777')
    assert '''content: '1'
path: /foo/bar
permissions: '0777'
''' == dump(
        subj
    )


def test_users():
    subj = Users(name='worker', keys=['ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQC7 worker@yandex-team.ru'])
    assert '''name: worker
shell: /bin/bash
ssh_authorized_keys:
- ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQC7 worker@yandex-team.ru
sudo: ALL=(ALL) NOPASSWD:ALL
''' == dump(
        subj
    )


def test_manage_etc_hosts():
    subj = UpdateEtcHosts(True)
    assert '''true
...
''' == dump(
        subj
    )


def test_hostname():
    subj = Hostname('a.db.yandex.net')
    assert '''a.db.yandex.net
...
''' == dump(
        subj
    )


def test_runcmd():
    subj = RunCMD('ls -la /home/$USER')
    assert '''ls -la /home/$USER
...
''' == dump(
        subj
    )


def test_legacy_key_value():
    subj = LegacyKeyValue('deploy_version', '2')
    assert '''deploy_version: '2'
''' == dump(
        subj
    )
