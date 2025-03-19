from google.protobuf import text_format
import pytest
import yatest.common

from cloud.disk_manager.internal.pkg.configs.client.config import config_pb2 as client_config
from cloud.disk_manager.internal.pkg.configs.server.config import config_pb2 as server_config


CLUSTERS = [
    "cloudvm/global/control",
    "hw-lab/common/global/control",
    "hw-lab/common/global/data",
    "hw-lab/hw-nbs-stable-lab/global/control",
    "hw-lab/hw-nbs-stable-lab/global/data",
    "hw-lab/seed/control",
    "israel/m1a/control",
    "israel/m1a/data",
    "israel/seed/control",
    "preprod/myt/control",
    "preprod/myt/data",
    "preprod/sas/control",
    "preprod/sas/data",
    "preprod/vla/control",
    "preprod/vla/data",
    "prod/myt/control",
    "prod/myt/data",
    "prod/sas/control",
    "prod/sas/data",
    "prod/vla/control",
    "prod/vla/data",
    "testing/myt/control",
    "testing/myt/data",
    "testing/sas/control",
    "testing/sas/data",
    "testing/seed/control",
    "testing/vla/control",
    "testing/vla/data",
    "vagrant/global",
]


def get_config_file(cluster, name):
    return yatest.common.test_source_path("../{}/{}.txt".format(cluster, name))


def parse_config_file(pb, cluster, name):
    with open(get_config_file(cluster, name), "r") as f:
        text_format.Parse(f.read(), pb)


@pytest.mark.parametrize("cluster", CLUSTERS)
def test_server_config(cluster):
    pb = server_config.ServerConfig()
    parse_config_file(pb, cluster, "server-config")


@pytest.mark.parametrize("cluster", CLUSTERS)
def test_client_config(cluster):
    pb = client_config.ClientConfig()
    parse_config_file(pb, cluster, "client-config")
