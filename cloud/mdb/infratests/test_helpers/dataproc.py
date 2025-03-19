from typing import Sequence

import semver
import sh

from cloud.mdb.internal.python.compute.instances import InstanceModel

from cloud.mdb.infratests.test_helpers import py_api
from cloud.mdb.infratests.test_helpers.compute import get_compute_api
from cloud.mdb.infratests.test_helpers.context import Context


def execute_command_on_master_node(context: Context, command: str, timeout: int = 900):
    master_node = next(host for host in context.hosts if host['role'] == 'MASTERNODE')
    master_node_hostname = master_node['name']

    ssh_options = ['-o', 'StrictHostKeyChecking=no', '-o', 'UserKnownHostsFile=/dev/null']
    ssh_options += ['-i', context.test_config.dataproc.ssh_key.private_path]

    sh.ssh(*ssh_options, f'vlbel@{master_node_hostname}', command, _timeout=timeout)


def get_compute_instances_of_cluster(context: Context, view: str = 'BASIC') -> Sequence[InstanceModel]:
    """
    Returns the list of compute instances of the cluster.
    """
    compute_api = get_compute_api(context)
    return [compute_api.get_instance(instance_id=host['computeInstanceId'], view=view) for host in context.hosts]


def get_compute_instances_of_subcluster(context: Context, subcluster_name: str):
    """
    Fetches all instance objects of subcluster from compute api.
    """
    hosts = py_api.get_hosts_of_subcluster(context, subcluster_name)
    compute_api = get_compute_api(context)
    return [compute_api.get_instance(fqdn=host['name'], instance_id=host['computeInstanceId']) for host in hosts]


def get_default_dataproc_version(context: Context, version_prefix: str = None) -> str:
    cluster_config = py_api.get_console_cluster_config(context).json()
    if not version_prefix:
        version_prefix = cluster_config['defaultVersion']

    versions = cluster_config['versions']

    dataproc_version = None
    for version in versions:
        if version.startswith(version_prefix):
            try:
                parsed_version = semver.VersionInfo.parse(version)
                if not dataproc_version or parsed_version >= dataproc_version:
                    dataproc_version = version
            except ValueError:
                # skip not semver compatible old versions like 2.0
                pass

    if not dataproc_version:
        raise RuntimeError(f'No version in {versions} match prefix {version_prefix}')

    return str(dataproc_version)


def get_default_dataproc_version_prefix(context: Context) -> str:
    cluster_config = py_api.get_console_cluster_config(context).json()
    return str(cluster_config['defaultVersion'])
