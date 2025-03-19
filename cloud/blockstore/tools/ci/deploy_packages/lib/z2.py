import os

from cloud.blockstore.pylibs.clients.z2 import Z2Client
from cloud.blockstore.pylibs.clusters.update_config import ClusterUpdateConfig

from .errors import Error


_BINARY_PACKAGES = {
    'blockstore': {
        'yandex-cloud-blockstore-client': True,
        'yandex-cloud-blockstore-disk-agent': False,
        'yandex-cloud-blockstore-http-proxy': True,
        'yandex-cloud-blockstore-nbd': False,
        'yandex-cloud-blockstore-server': True,
        'yandex-cloud-blockstore-plugin': False,
        'yandex-cloud-storage-breakpad': True,
        'yc-nbs-disk-agent-systemd': False,
        'yc-nbs-breakpad-systemd': True,
        'yc-nbs-http-proxy-systemd': True,
        'yc-nbs-systemd': True,
    },

    'filestore': {
        'yandex-cloud-filestore-client': True,
        'yandex-cloud-filestore-http-proxy': False,
        'yandex-cloud-filestore-server': True,
        'yandex-cloud-filestore-vhost': False,
        'yandex-cloud-storage-breakpad': True,
        'yc-nfs-http-proxy-systemd': False,
        'yc-nfs-server-systemd': True,
        'yc-nfs-vhost-systemd': False,
        'yc-nfs-breakpad-systemd': True,
    },

    'snapshot': {
        'yc-snapshot': True,
    },
}

_CONFIG_PACKAGES = {
    'blockstore': {
        'yandex-search-kikimr-nbs-conf-hw-nbs-dev-lab-global': False,
        'yandex-search-kikimr-nbs-conf-hw-nbs-stable-lab-global': False,
        'yandex-search-kikimr-nbs-control-conf-hw-nbs-stable-lab-global': True,
    },

    'filestore': {
        'yandex-search-kikimr-nfs-conf-hw-nbs-dev-lab-global': False,
        'yandex-search-kikimr-nfs-conf-hw-nbs-stable-lab-global': False,
        'yandex-search-kikimr-nfs-control-conf-hw-nbs-stable-lab-global': True,
    },
}


class Z2Helper:
    def __init__(self, cluster: ClusterUpdateConfig, logger):
        self._cluster = cluster
        self._logger = logger

        z2_token = os.getenv('Z2_TOKEN')
        if z2_token is None:
            raise Error('no Z2_TOKEN specified')

        self._z2 = Z2Client(z2_token, logger, force=True)

    def edit_package_versions(self, new_packages, service):
        binaries = _BINARY_PACKAGES.get(service, {})
        configs = _CONFIG_PACKAGES.get(service, {})

        packages = []
        control_packages = []

        for package in new_packages:
            name = package['name']

            if name in configs:
                if name.find(self._cluster.name) != -1:
                    if configs[name]:
                        self._z2.edit(self._cluster.z2_control_conf_meta_group, [package])
                    else:
                        self._z2.edit(self._cluster.z2_conf_meta_group, [package])

            if name in binaries:
                packages.append(package)

                if binaries[name]:
                    control_packages.append(package)

        if packages:
            self._z2.edit(self._cluster.z2_meta_group, packages)

        if control_packages and self._cluster.z2_control_meta_group is not None:
            self._z2.edit(self._cluster.z2_control_meta_group, control_packages)

    def run_update(self):
        self._z2.update_sync(self._cluster.z2_group)
        if self._cluster.z2_control_group is not None:
            self._z2.update_sync(self._cluster.z2_control_group)
