import copy
import re

import pytest

from yc_issue_cert.secrets import SecretData
from yc_issue_cert.secret_service import SecretServiceCli, SecretServiceConfig, Secret

PREFIX = "cloudvm_oct"
CA_SECRET = {
    "id": "f1b78fb3-7e57-11ea-ac71-374ec60ef4d0",
    "name": "cloudvm_oct_ca",
    "files": [
        {"name": "ca.crt"}
    ]
}
API_CERT_SECRET = {
    "id": "24349feb-7e57-11ea-9d2d-989657f66734",
    "name": "cloudvm_oct_oct-head1-api-cert",
    "files": [
        {"name": "oct-head1.cloud-lab.yandex.net.crt"}
    ]
}
CA_SECRET_EXT = {
    "status": "OK",
    "secret": {
        "id": CA_SECRET["id"],
        "name": CA_SECRET["name"],
        "files": [
            {
                "name": CA_SECRET,
                "versions": [
                    {
                        # Dummy value (update it with actual value from by ca_secret fixture)
                        "sha256": 'm5hxuX0yYKy0s/pc3eVGJP9Qxn5UElJNzKsiOoJzAyY=\n',
                        "user": "contrail",
                        "group": "contrail",
                        "version": "1",
                    }
                ]
            }
        ]
    },
    "usergroups": [
        {
            "name": "abc_ycvpc",
            "role": "admin"
        }
    ]
}

CA_SECRET_HOSTGROUP = "bootstrap_base-role_dev_cloudvm"
CA_SECRET_HOSTGROUPS = {
    "status": "OK",
    "hostgroups": [
        {
            "name": CA_SECRET_HOSTGROUP
        }
    ]
}


class DummySecretServiceCli(SecretServiceCli):
    def __init__(self):
        config = SecretServiceConfig({
            "allowed_base_roles": ["oct-head"],
            "base_role_abc_groups": {
                "oct-head": ["abc_ycvpc"],
            },
        })
        super().__init__(config, "testing")

        self.commands = {}
        self.re_commands = {}

    def run(self, *cmdargs):
        cmd = " ".join(cmdargs)
        result = {}
        for re_command, re_result in self.re_commands.items():
            if re.match(re_command, cmd):
                result = re_result
                break

        value = self.commands.setdefault(cmd, result)
        if callable(value):
            value = value(self)

        return value


class DummyCaCertData(SecretData):
    USER = "contrail"
    GROUP = "contrail"


@pytest.fixture
def cli():
    return DummySecretServiceCli()


@pytest.fixture
def ca_data(ca_cert_path):
    return DummyCaCertData("ca", ca_cert_path, "/etc/ssl/ca.crt",
                           bootstrap_filter={"roles": ["oct-head"]},
                           ss_hostgroups=[("bs_base-role_dev_oct-head", "oct-head")])


@pytest.fixture
def ca_secret(cli, ca_data):
    return Secret(cli, ca_data, CA_SECRET["name"])


@pytest.fixture
def api_cert_data(api_cert_path):
    return SecretData("oct-head1-api-cert", api_cert_path,
                      "/etc/ssl/oct-head1.cloud-lab.yandex.net",
                      bootstrap_filter={"hosts": "oct-head1.cloud-lab.yandex.net"},
                      ss_hostgroups=[("bs_base-role_dev_oct-head", "oct-head")])


@pytest.fixture
def api_cert_secret(cli, api_cert_data):
    return Secret(cli, api_cert_data, API_CERT_SECRET["name"])


def test_resolve_secrets(cli, ca_data, api_cert_data):
    cli.commands = {
        "secret list": {
            "status": "OK",
            "secrets": [CA_SECRET, API_CERT_SECRET],
        }
    }

    secrets = Secret.resolve_secrets(cli, [ca_data, api_cert_data], PREFIX)

    assert len(secrets) == 2
    assert secrets[0].data is ca_data
    assert secrets[0].id == CA_SECRET["id"]
    assert secrets[1].data is api_cert_data
    assert secrets[1].id == API_CERT_SECRET["id"]


def test_secret_upsert_1(cli, ca_secret):
    cli.re_commands[r"secret add -f \S+/ca.crt -n cloudvm_oct_ca"] = {
        "status": "CREATED",
        "id": CA_SECRET["id"],
    }

    ca_secret.upsert()

    assert cli.commands


def test_secret_upsert_2(cli, api_cert_secret):
    api_cert_secret.id = API_CERT_SECRET["id"]
    api_cert_secret.upsert()

    assert not cli.commands


@pytest.mark.parametrize("alter_func, expected_cmd", [
    # Secret is up to date, no commands are expected
    (lambda s, v, h: None,
     None),

    (lambda s, v, h: s.update({"usergroups": []}),
     "secret grant {} -g abc_ycvpc -r admin".format(CA_SECRET["id"])),
    (lambda s, v, h: v.update({"user": "root"}),
     ("secret set {} --version 1 --filename ca.crt"
      " --user contrail --group contrail").format(CA_SECRET["id"])),
    (lambda s, v, h: h.update({"hostgroups": []}),
     "hostgroup add-secret {} --secret-id {}".format(CA_SECRET_HOSTGROUP, CA_SECRET["id"])),
])
def test_secret_sync_assign(alter_func, expected_cmd, cli, ca_secret):
    ca_secret_ext = copy.deepcopy(CA_SECRET_EXT)
    ca_secret_ver = ca_secret_ext["secret"]["files"][0]["versions"][0]
    ca_secret_ver["sha256"] = ca_secret.data.get_sha256_b64()
    ca_secret_hostgroups = copy.deepcopy(CA_SECRET_HOSTGROUPS)

    alter_func(ca_secret_ext, ca_secret_ver, ca_secret_hostgroups)

    cli.commands = {
        "secret show {}".format(CA_SECRET["id"]): ca_secret_ext,
        "secret list-hostgroups {}".format(CA_SECRET["id"]): ca_secret_hostgroups,
    }

    ca_secret.id = CA_SECRET["id"]
    ca_secret.upsert()
    ca_secret.load()
    ca_secret.sync()
    ca_secret.assign(CA_SECRET_HOSTGROUP, "oct-head")

    if expected_cmd:
        assert expected_cmd in cli.commands


def test_secret_update(cli, ca_secret):
    def update_side_effect(cli):
        nonlocal show_command

        ca_secret_ext = copy.deepcopy(CA_SECRET_EXT)
        ca_secret_newver = copy.deepcopy(ca_secret_ext["secret"]["files"][0]["versions"][0])
        ca_secret_newver["sha256"] = ca_secret.data.get_sha256_b64()
        ca_secret_newver["version"] = "2"
        ca_secret_ext["secret"]["files"][0]["versions"].append(ca_secret_newver)

        cli.commands[show_command] = ca_secret_ext

        return {"status": "OK"}

    show_command = "secret show {}".format(CA_SECRET["id"])
    cli.commands = {
        show_command: CA_SECRET_EXT,
        "secret update {} -f {}".format(CA_SECRET["id"], ca_secret.data.srcpath): update_side_effect,
        "secret list-hostgroups {}".format(CA_SECRET["id"]): CA_SECRET_HOSTGROUPS,
    }

    ca_secret.id = CA_SECRET["id"]
    ca_secret.upsert()
    ca_secret.load()
    ca_secret.sync()

    assert ca_secret._find_versioned_file()["version"] == "2"
