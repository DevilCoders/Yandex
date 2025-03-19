"""
Utilities for dealing with Hadoop cluster
"""
import datetime
import gzip
import json
import logging
import os
import random
import shutil
import string
import sys
import threading
from collections import defaultdict
from typing import Dict

import semver
import yaml
from retrying import retry

from tests.helpers import internal_api
from tests.helpers.compute import ComputeApi
from tests.helpers.compute_driver import get_driver
from tests.helpers.utils import FakeContext, merge, remote_yaml_write, rsync, ssh
from yandex.cloud.priv.compute.v1 import instance_pb2

LOG = logging.getLogger('hadoop_cluster')


class HadoopClusterException(Exception):
    """
    Hadoop Cluster Exception
    """


def get_default_dataproc_version_prefix(context):
    cluster_config = internal_api.get_console_cluster_config(context, folder_id=context.folder['folder_ext_id']).json()
    return str(cluster_config['defaultVersion'])


def get_default_dataproc_version(context, version_prefix: str = None):
    cluster_config = internal_api.get_console_cluster_config(context, folder_id=context.folder['folder_ext_id']).json()
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


class HadoopCluster:
    def __init__(self, context):
        self.context = context
        self.gateway = get_driver(context.state, context.conf).get_managed_ipv6_address()
        master_node = next(host for host in context.hosts if host['role'] == 'MASTERNODE')
        self.master_node_hostname = master_node['name']
        self.hostnames = [host['name'] for host in context.hosts]
        ssh_key_path = context.conf['compute_driver']['ssh_keys']['dataplane']['private']
        self.ssh_options = f'-i {ssh_key_path} -q -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'
        self.username = 'root'
        if hasattr(context, 'username'):
            self.username = context.username

    def update_dataproc_agent(self):
        self.stop_dataproc_agent()
        self.update_dataproc_agent_binary()
        self.update_dataproc_agent_config()
        self.start_dataproc_agent()

    def create_hdfs_file(self, path: str, size: str):
        create_file_command = f'head -c {size} /dev/urandom > /tmp/random_file'
        get_file_checksum = 'md5sum /tmp/random_file'
        upload_file_command = f'sudo -u hdfs hdfs dfs -put -f /tmp/random_file {path}'
        command = f'{create_file_command} && {get_file_checksum} && {upload_file_command}'
        stdout, stderr = self.on_gateway(f'ssh {self.ssh_options} {self.master_node_hostname} "{command}"')
        hash_sum = stdout.split()[0]
        assert len(hash_sum) == 32
        return hash_sum, stderr

    def get_hdfs_file_hash_sum(self, path: str):
        command = f'sudo -u hdfs hdfs dfs -get -f {path} /tmp/file_from_hdfs && md5sum /tmp/file_from_hdfs'
        stdout, stderr = self.on_gateway(f'ssh {self.ssh_options} {self.master_node_hostname} "{command}"')
        hash_sum = stdout.split()[0]
        assert len(hash_sum) == 32
        return hash_sum, stderr

    def check_hdfs_file_block_locations(self, path: str):
        get_block_locations_url = f'http://{self.master_node_hostname}:9870/webhdfs/v1{path}?op=GETFILEBLOCKLOCATIONS'
        get_block_locations = f'curl "{get_block_locations_url}"'
        stdout, err = self.on_gateway(f'ssh {self.ssh_options} {self.master_node_hostname} "{get_block_locations}"')
        blocks_by_nodes = defaultdict(set)
        block_locations = json.loads(stdout)["BlockLocations"]["BlockLocation"]
        total_blocks_number = len(block_locations)
        does_any_nodes_contain_all_blocks = False
        for block_index, block_location in enumerate(block_locations):
            for host in block_location["hosts"]:
                blocks_by_nodes[host].add(block_index)
        for host, blocks in blocks_by_nodes.items():
            if len(blocks) == total_blocks_number:
                does_any_nodes_contain_all_blocks = True
        return does_any_nodes_contain_all_blocks, stdout

    @retry(
        stop_max_attempt_number=6,
        retry_on_exception=lambda exc: isinstance(exc, HadoopClusterException),
        wait_exponential_multiplier=1000,
        wait_exponential_max=10000,
    )
    def on_gateway(self, cmd):
        code, out, err = ssh(self.gateway, [cmd])
        if code != 0:
            raise HadoopClusterException(f'Failed to execute {cmd} on gateway, code: {code}, out: {out}, err: {err}')
        return out, err

    def ssh_to_master(self):
        cmd = f'ssh {self.ssh_options} {self.master_node_hostname}'
        code, out, err = ssh(self.gateway, [cmd], options=['-tt'], stdout=sys.stdout, stderr=sys.stderr, timeout=None)
        if code != 0:
            raise HadoopClusterException(f'Failed to execute {cmd} on gateway, code: {code}, out: {out}, err: {err}')

    def stop_dataproc_agent(self):
        self.on_gateway(
            f'ssh {self.ssh_options} {self.username}@{self.master_node_hostname}'
            ' "supervisorctl stop dataproc-agent >/dev/null"'
        )

    def start_dataproc_agent(self):
        self.on_gateway(
            f'ssh {self.ssh_options} {self.username}@{self.master_node_hostname}'
            ' "supervisorctl start dataproc-agent >/dev/null"'
        )

    def update_dataproc_agent_binary(self):
        code, out, err = rsync(
            '../dataproc-agent/cmd/dataproc-agent/dataproc-agent',
            f'[{self.gateway}]:/tmp/dataproc-agent',
            additional_options=['-L'],
        )
        if code != 0:
            raise HadoopClusterException(
                f'Failed to sync dataproc-agent binary to gateway, code: {code}, out: {out}, err: {err}'
            )
        self.on_gateway(
            f'scp {self.ssh_options} /tmp/dataproc-agent {self.username}@{self.master_node_hostname}'
            ':/opt/yandex/dataproc-agent/bin/dataproc-agent'
        )

    def update_dataproc_agent_config(self):
        self.on_gateway(
            f'scp {self.ssh_options} {self.username}@{self.master_node_hostname}:'
            '/opt/yandex/dataproc-agent/etc/dataproc-agent.yaml /tmp/'
        )

        code, out, err = rsync(f'[{self.gateway}]:/tmp/dataproc-agent.yaml', '/tmp/dataproc-agent.yaml')
        if code != 0:
            raise HadoopClusterException(
                f'Failed to download dataproc-agent.yaml from gateway to local machine,'
                f'code: {code}, out: {out}, err: {err}'
            )
        with open('/tmp/dataproc-agent.yaml') as agent_config_file:
            dataproc_agent_config = yaml.safe_load(agent_config_file)

        config_override = self.context.conf.get('dataproc_agent_config_override', {})
        merge(dataproc_agent_config, config_override)
        remote_yaml_write(dataproc_agent_config, f'[{self.gateway}]', '/tmp/dataproc-agent.yaml')

        self.on_gateway(
            f'scp {self.ssh_options} /tmp/dataproc-agent.yaml {self.username}@{self.master_node_hostname}:'
            '/opt/yandex/dataproc-agent/etc/dataproc-agent.yaml'
        )

    def download_dataproc_agent_log(self, path):
        self.on_gateway(
            f'ssh {self.ssh_options} {self.username}@{self.master_node_hostname} '
            'gzip -kf /var/log/supervisor/dataproc-agent.log'
        )

        self.on_gateway(
            f'scp {self.ssh_options} {self.username}@{self.master_node_hostname}:'
            '/var/log/supervisor/dataproc-agent.log.gz /tmp/'
        )

        archive_path = f'/tmp/dataproc-agent-{os.getpid()}.log.gz'
        code, out, err = rsync(f'[{self.gateway}]:/tmp/dataproc-agent.log.gz', archive_path)
        if code != 0:
            raise HadoopClusterException(
                f'Failed to download dataproc-agent.log from gateway to local machine,'
                f'code: {code}, out: {out}, err: {err}'
            )

        with gzip.open(archive_path, 'rb') as f_in:
            with open(path, 'wb') as f_out:
                shutil.copyfileobj(f_in, f_out)
        os.remove(archive_path)

    def download_diagnostics(self, diagnostics_path):
        threads = []
        for hostname in self.hostnames:
            thread = threading.Thread(target=self.diagnostics_or_snapshot, args=(hostname, diagnostics_path))
            threads.append(thread)
            thread.start()
        for thread in threads:
            thread.join(5 * 60)

    def diagnostics_or_snapshot(self, hostname, diagnostics_path):
        try:
            self.download_diagnostics_from_host(hostname, diagnostics_path)
        except HadoopClusterException:
            LOG.info(f'Failed to take diagnostics from {hostname}. Will try to create snapshot.')
            self.create_snapshot(hostname)
            raise

    def download_diagnostics_from_host(self, hostname, diagnostics_path):
        logging.info(f"Collecting diagnostics for {hostname} ...")
        random_id = ''.join(random.choice(string.ascii_lowercase) for _ in range(10))
        filename = f'/tmp/diagnostics-{random_id}.tgz'
        # collect diagnostics with 2 optimizations:
        # * do not verify integrity of packages (it is too long running)
        # * collect diagnostics only once per cluster. It is sufficient to collect diagnostics
        #   after the first failed task. The following errors will most likely be the result
        #   of the first one.
        cmd = (
            "if [ ! -f 'diagnostics.tgz' ]; then "
            "  DPKG_VERIFY=false sudo /usr/local/bin/dataproc-diagnostics.sh "
            "else "
            "  echo 'exists'; "
            "fi"
        )
        out, err = self.on_gateway(f'ssh {self.ssh_options} {self.username}@{hostname} "{cmd}"')
        if 'exists' in str(out):
            return
        logging.info(f"Downloading diagnostics for {hostname} ...")
        home = '/root'
        if self.username != 'root':
            home = f'/home/{self.username}'
        self.on_gateway(f'scp {self.ssh_options} {self.username}@{hostname}:{home}/diagnostics.tgz {filename}')
        path = os.path.join(diagnostics_path, f"{hostname.split('.')[0]}.diagnostics.tgz")
        code, out, err = rsync(f'[{self.gateway}]:{filename}', path)
        if code != 0:
            raise HadoopClusterException(
                f'Failed to download diagnostics.tgz from gateway to local machine,'
                f'code: {code}, out: {out}, err: {err}'
            )

    def run_command(self, command):
        self.on_gateway(f'ssh {self.ssh_options} {self.username}@{self.master_node_hostname} "{command}"')

    def create_snapshot(self, hostname):
        """
        Create snapshot of disk of VM
        """
        # details are within https://st.yandex-team.ru/MDB-9145
        compute_api = ComputeApi(self.context.conf['compute'], LOG)
        host = next(h for h in self.context.hosts if h['name'] == hostname)
        instance_id = host['computeInstanceId']
        instance = compute_api.get_instance(hostname, instance_id)
        if not instance:
            LOG.info(f'Failed to find instance {hostname} {instance_id}')
            return

        if instance.status != instance_pb2.Instance.Status.RUNNING:
            LOG.info(f'Instance {hostname} {instance_id} is not running')
            return

        LOG.info(f'Stopping instance {hostname}')
        compute_api.compute_operation_wait(compute_api.instance_stopped(hostname, instance.id))

        keep_until = datetime.datetime.now() + datetime.timedelta(days=7)
        labels = {
            'cluster_id': self.context.cluster['id'],
            'subcluster_id': host['subclusterId'],
            'fqdn': hostname,
            'instance_id': instance_id,
            'arcanum_id': os.environ.get('ARCANUMID', ''),
            'jenkins_build': os.environ.get('BUILD_NUMBER', ''),
            'owner': self.context.conf['user'],
            'keep_until': str(int(keep_until.timestamp())),
        }

        LOG.info(f"Taking snapshot of disk {instance.boot_disk.disk_id} from instance {hostname}")
        operation_id, snapshot_id = compute_api.create_snapshot(instance.boot_disk.disk_id, instance.folder_id, labels)
        compute_api.compute_operation_wait(operation_id)
        LOG.info(f'Created snapshot {snapshot_id}')

        LOG.info(f'Starting instance {hostname}')
        compute_api.compute_operation_wait(compute_api.instance_running(hostname, instance.id))

    def ssh_for_cluster_hosts(self, cluster_hosts, command: str):
        result = {}
        for fqdn in cluster_hosts:
            out, err = self.on_gateway(f'ssh {self.ssh_options} {fqdn} "{command}"')
            result[fqdn] = (out, err)
        return result


def ssh_to_master(state: Dict, conf: Dict, **_) -> None:
    try:
        context = FakeContext(state, conf)
        internal_api.ensure_cluster_is_loaded_into_context(context)
        hadoop_cluster = HadoopCluster(context)
        logging.info(f"connecting to {hadoop_cluster.master_node_hostname} ...")
        hadoop_cluster.ssh_to_master()
    except KeyboardInterrupt:
        logging.info("Bye-bye!")
