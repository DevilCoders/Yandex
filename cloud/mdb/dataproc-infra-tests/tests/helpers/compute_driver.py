#!/usr/bin/env python3
"""
Create control-plane inside one VM or container
"""

import json
import logging
import os
import shlex
import socket
import subprocess
import time
from dataclasses import dataclass
from datetime import datetime, timedelta, timezone
from enum import Enum
from typing import Dict, List, Union, Set

import yaml
from retrying import retry

from .compute import ComputeApi
from .dns import DnsApi, Record
from .instance_group import InstanceGroupApi
from .metadb import get_all_hosts, truncate_clusters_cascade, truncate_worker_queue_cascade
from .pillar import get_pillar, mdb_internal_api_pillar
from .s3 import delete_bucket, list_buckets, put_object
from .ssh_pki import gen_pair
from .utils import (
    FakeContext,
    download,
    env_stage,
    fix_rel_path,
    remote_file_write,
    remote_yaml_write,
    rsync,
    rsync_checked,
    ssh,
    ssh_checked,
    get_env_without_python_entry_point,
)

LOG = logging.getLogger('compute_driver')
GiB = 2**30

DEFAULT_RESOURCES = {
    'zone': 'ru-central1-a',
    'image_type': 'dataproc-infratest',  # Prebuild image based on common-2if
    'platform': 'standard-v2',
    'cores': 2,
    'core_fraction': 20,
    'memory': 4 * GiB,
    'root_disk_size': 20 * GiB,
}

ROBOT_PGAAS_CI_PUBLIC_KEY = (
    "ecdsa-sha2-nistp256 AAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBE/"
    "wvOhla5vCLimxbKyKgaKI0Wp6agsbcGwxmr4YKpmu97B4yfetDPLTG5QQBdzT7TIBngfPYpnYsFRaTVjvt3E= robot-pgaas-ci"
)


class ComputeDriverError(Exception):
    """
    Base Driver Exception
    """


class ComputeDriver:
    """
    Special driver for creating control-plane VM in Compute API
    """

    def __init__(self, config: Dict, fqdn: str, dataplane_folder_id: str, network_id: str, resources: Dict, owner: str):
        self.compute_api = ComputeApi(config['compute'], LOG)
        self.dns_api = DnsApi(config['dns'], LOG)
        self.subnet_id = self.compute_api.get_geo_subnet(dataplane_folder_id, network_id, resources['zone']).id
        self.fqdn = fqdn
        self.dns_name = fqdn if fqdn[-1] == '.' else fqdn + '.'
        self.managed_address = None
        self.external_address = None
        self.resources = resources
        self.dataplane_folder_id = dataplane_folder_id
        self.owner = owner

    def absent(self) -> None:
        operation = self.compute_api.instance_absent(self.fqdn, folder_id=self.dataplane_folder_id)
        if operation:
            self.compute_api.compute_operation_wait(operation)
            self._unset_dns_records()

    def exists(self, image_type=None) -> None:
        if not image_type:
            image_type = self.resources['image_type']
        operation = self.compute_api.instance_exists(
            self.resources['zone'],  # zone
            self.fqdn,  # FQDN
            image_type,  # image type
            self.resources['platform'],  # hw platform
            self.resources['cores'],  # cores
            self.resources['core_fraction'],  # core fraction
            self.resources['memory'],  # memory in bytes
            self.subnet_id,  # subnet
            root_disk_size=self.resources['root_disk_size'],  # root disk size
            root_disk_type_id='network-ssd',
            folder_id=self.dataplane_folder_id,
            labels={
                'owner': self.owner,
            },
            metadata={'user-data': self._userdata()},
        )
        if operation:
            self.compute_api.compute_operation_wait(operation)
        self._set_dns_records()
        instance = self.get_instance()
        operation = self.compute_api.instance_running(self.fqdn, instance.id)
        if operation:
            self.compute_api.compute_operation_wait(operation)
        self._wait_ssh()

    def _userdata(self) -> None:
        return '#cloud-config\n' + yaml.safe_dump(
            {
                'system_info': {'default_user': '~'},
                'users': [{'name': 'root', 'ssh-authorized-keys': [ROBOT_PGAAS_CI_PUBLIC_KEY]}],
                'disable_root': False,
                'cloud_init_modules': ['users-groups'],
                'cloud_config_modules': [],
                'cloud_final_modules': [],
            },
            default_flow_style=False,
        )

    def _set_dns_records(self) -> None:
        self.dns_api.set_records(self.dns_name, [Record(address=self.get_managed_ipv6_address(), record_type='AAAA')])

    def _unset_dns_records(self) -> None:
        self.dns_api.set_records(self.dns_name, [])

    def stopped(self) -> None:
        instance = self.get_instance()
        operation = self.compute_api.instance_stopped(self.fqdn, instance.id)
        if operation:
            self.compute_api.compute_operation_wait(operation)

    def get_instance(self):
        return self.compute_api.get_instance(self.fqdn, folder_id=self.dataplane_folder_id)

    def get_fqdn(self) -> str:
        return self.fqdn

    @retry(wait_fixed=5000, stop_max_attempt_number=5, stop_max_delay=60000)
    def get_managed_ipv6_address(self) -> str:
        """
        Return internal ipv6 address for control-plane things
        """
        if self.managed_address:
            return self.managed_address

        internal_iface = self.get_instance().network_interfaces[-1]
        self.managed_address = internal_iface.primary_v6_address.address
        return self.managed_address

    def get_host(self) -> str:
        host = self.get_managed_ipv6_address()
        return f'[{host}]'

    @retry(wait_fixed=5000, stop_max_attempt_number=5, stop_max_delay=60000)
    def get_external_ipv4_address(self) -> Union[str, None]:
        """
        Return external ipv4 address for data-plane things.
        """
        if self.external_address:
            return self.external_address

        external_iface = self.get_instance().network_interfaces[0]
        self.external_address = external_iface.primary_v4_address.address
        return self.external_address

    def save_logs(self, logs_dir) -> None:
        """
        Collecting logs for every docker container
        """
        host = self.get_host()
        addr = self.get_managed_ipv6_address()

        ssh_checked(
            addr,
            [
                'for i in $(docker ps -a -q --format "{{.Names}}") ; do '
                'docker logs $i > /var/log/docker-log-container-$i.log 2>&1 ; '
                'done',
            ],
            message='Can\'t dump logs from docker containers',
        )
        code, out, err = download(f'{host}:/var/log/*', logs_dir)
        if code != 0:
            raise ComputeDriverError(f'Can\'t download logs from docker containers, stdout: {out}, stderr: {err}')

    @retry(wait_fixed=5000, stop_max_attempt_number=180)
    def wait_dns(self):
        """
        Wait until dns record will be available
        """
        try:
            socket.getaddrinfo(self.fqdn, 0)
            LOG.info(f'Waiting availability of dns record {self.fqdn} success')
        except (socket.gaierror, socket.timeout):
            LOG.info(f'Waiting availability of dns record {self.fqdn} failure')
            raise

    @retry(wait_fixed=5000, stop_max_attempt_number=120, retry_on_exception=lambda _: True)
    def _wait_ssh(self):
        """
        Wait until instance will listen ssh port
        """
        addr = self.get_managed_ipv6_address()
        code, out, err = ssh(addr, ['uptime > /dev/null'], timeout=15)
        success = 'success'
        if code != 0:
            success = 'failure'
        else:
            err = err[:50]
        msg = f'Waiting ssh connectivity {success}, code: {code}, stderr: {err}'
        LOG.debug(msg)
        msg = f'Waiting ssh connectivity {success}, code: {code}'
        LOG.info(msg)
        if code != 0:
            raise ComputeDriverError(msg)

    def cache_image(self):
        """
        Cache control-plane vm image
        """
        instance = self.get_instance()
        disk_id = instance.boot_disk.disk_id
        date = time.strftime("%Y-%m-%d")
        unixtime = int(time.time())
        prefix = 'dbaas-dataproc-infratest'
        image_name = f'{prefix}-{date}-{unixtime}'
        operation = self.compute_api.image_exists(disk_id, image_name)
        if operation:
            self.compute_api.compute_operation_wait(operation.id)
            LOG.info('Created new cached image %s', image_name)

        # Remove old images
        images = sorted(self.compute_api.image_list(prefix=prefix), key=lambda image: image.name)
        if len(images) > 5:
            old_images = images[: len(images) - 5]
            operations = []
            for image in old_images:
                LOG.info('Deleting old cached image %s', image.name)
                operation = self.compute_api.image_absent(image.name)
                operations.append(operation)
            for operation in operations:
                self.compute_api.compute_operation_wait(operation.id)

    def delete_instances(self, instances: List[List[str]] = []) -> None:
        operations = []
        for fqdn, instance_id in instances:
            operation = self.compute_api.instance_absent(fqdn, instance_id)
            if operation:
                LOG.info('Deleting dataplane instance %s', fqdn)
                operations.append(operation)
        for operation in operations:
            self.compute_api.compute_operation_wait(operation)


@dataclass
class SyncComponent:
    aliases: Set[str]
    sync_by_default: bool = True


class SyncCodeComponents(Enum):
    Worker = SyncComponent({'worker', 'dbaas-worker'})
    DataprocAgent = SyncComponent({'agent', 'dataproc-agent'})
    DataprocManager = SyncComponent({'manager', 'dataproc-manager'})
    DataprocUIProxy = SyncComponent({'uiproxy', 'ui-proxy', 'dataproc-ui-proxy'})
    DbaasInternalAPI = SyncComponent({'intapi', 'internal-api'})
    MdbInternalAPI = SyncComponent({'gointapi', 'mdb-internal-api'})
    DeployAPI = SyncComponent({'deploy', 'deploy-api', 'mdb-deploy-api'}, sync_by_default=False)
    KafkaAgent = SyncComponent({'kafka-agent', 'topic-sync'})
    AdminApiGateway = SyncComponent({'adminapi-gateway'})
    Maintenance = SyncComponent({'maintenance'}, sync_by_default=False)

    def __str__(self):
        return f'{self.name}[{",".join(self.value.aliases)}]'

    @staticmethod
    def by_alias(alias: str) -> 'SyncCodeComponents':
        alias_map = {alias: component for component in SyncCodeComponents for alias in component.value.aliases}
        return alias_map[alias]

    @staticmethod
    def parse(component_aliases: str) -> Set['SyncCodeComponents']:
        if component_aliases == 'none':
            return set()
        if component_aliases is None:
            default_components = {component for component in SyncCodeComponents if component.value.sync_by_default}
            if os.environ.get('WITH_MDB_MAINTENANCE'):
                default_components.add(SyncCodeComponents.Maintenance)
            return default_components
        try:
            return {SyncCodeComponents.by_alias(alias.lower()) for alias in component_aliases.split(',')}
        except KeyError as e:
            raise RuntimeError(
                f'Unknown alias {e}, known components: {", ".join(map(str, SyncCodeComponents))}'
            ) from None


@env_stage('cache', fail=True)
def rebuilded(state: Dict, conf: Dict, **_) -> None:
    """
    Internal method for recreating control-plane virtual-machine for caching image
    """
    driver = get_driver(state, conf)
    driver.absent()
    driver.exists(image_type="common-2if")

    fqdn = driver.get_fqdn()
    ipv6 = driver.get_managed_ipv6_address()
    ipv4 = driver.get_external_ipv4_address()
    LOG.info('ComputeDriver host: %s, public ipv4: %s, managed ipv6: %s', fqdn, ipv4, ipv6)


@env_stage('create', fail=True)
def exists(state: Dict, conf: Dict, **_) -> None:
    """
    Create controlplane VM if it not exists
    """
    driver = get_driver(state, conf)
    driver.exists()

    fqdn = driver.get_fqdn()
    ipv6 = driver.get_managed_ipv6_address()
    ipv4 = driver.get_external_ipv4_address()
    LOG.info(f"ComputeDriver host: {fqdn}, public ipv4: {ipv4}, managed ipv6: {ipv6}")


@env_stage('stop', fail=True)
def stopped(state: Dict, conf: Dict, **_) -> None:
    """
    Stop controlplane virtual machine.
    """
    driver = get_driver(state, conf)

    # Remove dhcp cache before stop
    # Fix https://st.yandex-team.ru/CLOUD-4471
    ssh(driver.get_managed_ipv6_address(), ['rm /var/lib/dhcp/dhclient6.eth1.leases'])
    driver.stopped()


@env_stage('clean', fail=True)
def absent(state: Dict, conf: Dict, **_) -> None:
    """
    Stop controlplane virtual machine.
    """
    driver = get_driver(state, conf)
    driver.absent()


def get_driver(state: Dict, conf: Dict) -> ComputeDriver:
    """
    Return existing computeDriver or create new one.
    """
    driver = state.get('compute_driver')
    cfg = conf['compute_driver']
    if not driver:
        resources = {**DEFAULT_RESOURCES, **cfg.get('resources', {})}
        driver = ComputeDriver(
            conf, cfg['fqdn'], cfg['dataplane_folder_id'], cfg['network_id'], resources, conf['user']
        )
        state['compute_driver'] = driver
    return driver


def get_compute_api(context) -> ComputeApi:
    """
    Compute api get helper
    """
    return get_driver(context.state, context.conf).compute_api


def _get_managed_address(state: Dict, conf):
    """
    Return managed ip address
    """
    driver = get_driver(state, conf)
    return driver.get_managed_ipv6_address()


@env_stage('start', fail=True)
def build_pillar_regular(state: Dict, conf: Dict, **_) -> None:
    """
    Build salt pillar and environment for regular test run
    """
    runlist = [
        'components.web-api-base-lite',
        'components.nginx',
        'components.supervisor',
        'components.dbaas-worker',
        'components.postgres',
        'components.pg-dbs.dbaas_metadb',
        'components.dbaas-internal-api',
        'components.redis',
        'components.mdb-dataproc-manager',
        'components.fluentbit',
        'components.mdb-internal-api',
        'components.gateway',
        'components.grpcurl',
        'components.dataproc-ui-proxy',
    ]
    return _override_pillar(state, conf, runlist)


@env_stage('start_managed', fail=True)
def build_pillar_managed(state: Dict, conf: Dict, **_) -> None:
    """
    Build salt pillar and environment for regular test run
    """
    runlist = [
        'components.dataproc-infratests.speedup',
        'components.web-api-base-lite',
        'components.nginx',
        'components.supervisor',
        'components.dbaas-worker',
        'components.postgres',
        'components.pg-dbs.dbaas_metadb',
        'components.dbaas-internal-api',
        'components.redis',
        'components.mdb-dataproc-manager',
        'components.fluentbit',
        'components.mdb-internal-api',
        'components.gateway',
        'components.grpcurl',
        'components.deploy.salt-master',
        'components.pg-dbs.deploydb',
        'components.mocks.blackbox-mock',
        'components.mocks.juggler-mock',
        'components.mocks.solomon-mock',
        'components.mocks.certificator-mock',
        'components.deploy.mdb-deploy-api',
        'components.kafka_utils.kafkacat',
    ]
    if os.environ.get('WITH_MDB_MAINTENANCE'):
        runlist.append('components.mdb-maintenance')
    return _override_pillar(state, conf, runlist, managed=True)


@env_stage('start', fail=True)
def build_pillar_image(state: Dict, conf: Dict, **_) -> None:
    """
    Build salt pillar and environment for building caching image
    """
    runlist = [
        'components.dataproc-infratests.image',
        'components.web-api-base-lite',
        'components.nginx',
        'components.supervisor',
        'components.dbaas-worker',
        'components.postgres',
        'components.dbaas-internal-api',
        'components.mdb-dataproc-manager',
        'components.fluentbit',
        'components.mdb-internal-api',
        'components.gateway',
        'components.dataproc-ui-proxy',
        'components.mocks.blackbox-mock',
        'components.mocks.juggler-mock',
        'components.mocks.solomon-mock',
        'components.mocks.certificator-mock',
    ]
    conf['common']['is_cache'] = True
    return _override_pillar(state, conf, runlist)


def _override_pillar(state: Dict, conf: Dict, runlist: List[str], managed: bool = False) -> None:
    """
    Generate overriding pillar for forwarding meta information
    """
    driver = get_driver(state, conf)
    fqdn = driver.get_fqdn()
    addr = driver.get_managed_ipv6_address()
    host = driver.get_host()

    # Rewrite /etc/salt/minion_id, because this file may have incorrect value from image
    # Incorrect value affects on grains.id, and it will be brake salt states
    remote_file_write(fqdn, host, '/etc/salt/minion_id')

    # Write /srv/pillar/override.sls
    override = get_pillar(conf, fqdn, driver.get_external_ipv4_address(), runlist, managed)
    code, out, err = remote_yaml_write(override, host, "/srv/pillar/override.sls")
    if code != 0:
        raise ComputeDriverError(f'Can\'t write /srv/pillar/override.sls pillar, stdout: {out}, stderr: {err}')

    pillars = [
        'common',
        'mdb_controlplane_compute_preprod.mdb_meta_preprod',
        'mdb_controlplane_compute_preprod.mdb_api_preprod',
        'mdb_controlplane_compute_preprod.mdb_worker_preprod',
        'mdb_controlplane_compute_preprod.mdb_dataproc_manager_preprod',
        'mdb_controlplane_compute_preprod.mdb_dataproc_ui_proxy_preprod',
    ]
    if managed:
        pillars += [
            'mdb_controlplane_compute_preprod.deploy_salt.personal',
            'mdb_controlplane_compute_preprod.mdb_deploy_db_preprod',
            'mdb_controlplane_compute_preprod.mdb_deploy_api_preprod',
        ]
    pillars += ['override']
    # Rewrite /srv/pillar/top.sls
    code, out, err = remote_yaml_write({'base': {'*': pillars}}, host, "/srv/pillar/top.sls")
    if code != 0:
        raise ComputeDriverError(f'Can\'t write /srv/pillar/top.sls pillar, stdout: {out}, stderr: {err}')

    ssh_checked(addr, ['salt-call saltutil.clear_cache'])


@env_stage('cache', fail=True)
def disable_daemons(state: Dict, conf: Dict, **_) -> None:
    """
    Disable all services before making caching image.
    Needed for correct loading all services with highstate order.
    """
    driver = get_driver(state, conf)
    for service in ['redis-sentinel', 'redis-server', 'supervisor', 'nginx', 'docker', 'postgresql']:
        ssh(driver.get_host(), [f'systemctl disable {service}'])


@env_stage('cache', fail=True)
def cache_image(state: Dict, conf: Dict, **_) -> None:
    """
    Make new image from compute_driver and remove old ones.
    """
    driver = get_driver(state, conf)
    driver.cache_image()


@env_stage('cache', fail=True)
def rewrite_salt_configs(state: Dict, conf: Dict, **_) -> None:
    """
    Rewrite salt configs, disable master checking
    """
    driver = get_driver(state, conf)
    host = driver.get_host()
    addr = driver.get_managed_ipv6_address()

    code, out, err = remote_yaml_write(
        {
            'file_client': 'local',
            'log_level_logfile': 'info',
            'master_type': 'disable',
            'jinja_sls_env': {
                'trim_blocks': True,
            },
            'pillar_roots': {
                'base': ['/srv/pillar'],
                'qa': ['/srv/pillar'],
            },
            'file_roots': {
                'base': ['/srv/salt'],
                'dev': ['/srv/salt'],
                'qa': ['/srv/salt'],
            },
        },
        host,
        '/etc/salt/minion.d/yandex.conf',
    )
    if code != 0:
        raise ComputeDriverError(f'Can\'t rewrite salt-minion config, stdout: {out}, stderr: {err}')

    # Disable scheduled masterchecking
    ssh(addr, ['rm -rf /etc/salt/minion.d/_schedule.conf || exit 0'])


@env_stage('cache', fail=True)
def cleanup(state: Dict, conf: Dict, **_) -> None:
    """
    Current salt doen't know how to clean /etc/hosts, so we should remove cache fqdn before making image for tests.
    Just remove cache.infratest.db.yandex.net from /etc/hosts
    """
    driver = get_driver(state, conf)
    addr = driver.get_managed_ipv6_address()
    ssh_checked(addr, ['rm -rf /etc/redis/sentinel.conf'])
    ssh_checked(addr, ['sed -i "/cache.infratest.db.yandex.net/d" /etc/hosts'])
    ssh_checked(addr, ['rm -rf /etc/network/interfaces.d/*'])
    ssh_checked(addr, ['rm -rf /var/lib/dhcp/*'])
    ssh_checked(addr, ['rm -rf /etc/hostname'])


@env_stage('start', fail=True)
def sync_salt(state: Dict, conf: Dict, **_) -> None:
    """
    Sync staging salt code and pillar using rsync
    """
    driver = get_driver(state, conf)
    addr = driver.get_managed_ipv6_address()
    host = driver.get_host()
    rsync_checked('../salt/', f'{host}:/srv', message='Can\'t synchronize salt', timeout=120)
    rsync_checked('staging/code/salt/srv/pillar/mdb_controlplane_compute_preprod', f'{host}:/srv/pillar/')
    rsync_checked('staging/code/salt/srv/pillar/compute/preprod', f'{host}:/srv/pillar/compute')

    ssh_checked(addr, ['salt-call saltutil.clear_cache'])

    # Stop sentinel and remove his config for regenerate
    ssh(addr, ['service redis-sentinel stop || exit 0', 'rm -rf /etc/redis/sentinel.conf || exit 0'])
    # Ensure that python docker installed for salt module
    ssh_checked(addr, ['( dpkg-query -s python3-docker >/dev/null || apt install -y python3-docker )'])


@env_stage('start', fail=True)
def sync_salt_master(state: Dict, conf: Dict, **_) -> None:
    """
    Sync staging salt code and pillar using rsync
    """
    driver = get_driver(state, conf)
    addr = driver.get_managed_ipv6_address()
    fqdn = driver.get_host()
    ssh_checked(addr, ['mkdir -p /srv-master'])
    rsync_checked('../salt/', f'{fqdn}:/srv-master', timeout=120)
    ssh_checked(
        addr,
        ['mkdir -p /srv-master/envs && rm -rf /srv-master/envs/qa && ln -s /srv-master/salt /srv-master/envs/qa'],
        message='Can\'t create salt-master salt dir',
    )


def _remove_populate_tables(state: Dict, conf: Dict, **_) -> None:
    """
    Sync metadb grants
    """
    driver = get_driver(state, conf)
    ssh_checked(driver.get_managed_ipv6_address(), ['rm -rf /usr/local/yandex/dbaas_metadb_*.json'])


@env_stage('start', fail=True)
def sync_metadb(state: Dict, conf: Dict, **_) -> None:
    """
    Sync metadb code
    """
    driver = get_driver(state, conf)
    host = driver.get_host()
    rsync('../dbaas_metadb', f'{host}:/srv/salt/components/pg-code/', additional_options=['-L'])


@env_stage('start', fail=True)
def sync_deploydb(state: Dict, conf: Dict, **_) -> None:
    """
    Sync metadb code
    """
    driver = get_driver(state, conf)
    host = driver.get_host()
    rsync('../deploydb', f'{host}:/srv/salt/components/pg-code/', additional_options=['-L'])


def _rebuild_app(app_name, path, ya_cmd=None) -> None:
    """
    Rebuild arcadia project
    """
    LOG.info('Building new %s', app_name)
    ya_cmd = ya_cmd or 'ya'
    cmd = f'{ya_cmd} make -r --checkout --target-platform=linux --keep-going'
    # compile code on distbuild ferm
    if os.environ.get('USE_DISTBUILD'):
        cmd += ' --dist --download-artifacts'
    subprocess.check_call(shlex.split(cmd), cwd=(os.path.abspath(path)), env=get_env_without_python_entry_point())


def get_agent_version(conf):
    """
    Build a version for local dataproc-agent
    """
    ident = conf['compute_driver']['ident']
    return f'0.{ident}'


def _deploy_dataproc_agent(conf, src) -> None:
    """
    Deploy dataproc-agent to s3 bucket for new clusters
    """
    s3 = conf['s3']
    bucket = s3['bucket_name']
    version = get_agent_version(conf)
    dst = f'agent/dataproc-agent-{version}'
    put_object(s3, bucket, src, dst)
    LOG.info(f'Uploaded dataproc-agent to s3://{bucket}{dst}')


@env_stage('start', fail=True)
def sync_code(state: Dict, conf: Dict, **_) -> None:
    """
    Sync control plane code
    """
    driver = get_driver(state, conf)
    host = driver.get_host()
    addr = driver.get_managed_ipv6_address()
    ya_cmd = fix_rel_path(conf['common']['ya_command'])

    product = os.getenv('PRODUCT')

    sync_components = SyncCodeComponents.parse(os.getenv('SYNC_CODE_COMPONENTS'))

    if SyncCodeComponents.Worker in sync_components:
        _rebuild_app('dbaas-worker', f'{os.getcwd()}/../dbaas_worker/bin', ya_cmd)
        rsync_checked(
            '../dbaas_worker/bin/dbaas-worker',
            f'{host}:/opt/yandex/dbaas-worker/bin/dbaas-worker',
            additional_options=['-L'],
        )
        ssh(addr, ["chmod 755 /opt/yandex/dbaas-worker/bin/dbaas-worker"])
        rsync_checked('../dbaas_worker/dbaas-worker.conf', f'{host}:/etc/supervisor/conf.d/dbaas-worker.conf')

    if not product or product == 'dataproc':
        if SyncCodeComponents.DataprocAgent in sync_components:
            _rebuild_app('dataproc-agent', f'{os.getcwd()}/../dataproc-agent/', ya_cmd)
            _deploy_dataproc_agent(conf, '../dataproc-agent/cmd/dataproc-agent/dataproc-agent')

        if SyncCodeComponents.DataprocManager in sync_components:
            _rebuild_app('dataproc-manager', f'{os.getcwd()}/../dataproc-manager/', ya_cmd)
            rsync_checked(
                '../dataproc-manager/cmd/dataproc-manager/dataproc-manager',
                f'{host}:/opt/yandex/dataproc-manager/bin/dataproc-manager',
                additional_options=['-L'],
            )
            ssh(addr, ["chmod 755 /opt/yandex/dataproc-manager/bin/dataproc-manager"])
            ssh(addr, ["setcap 'cap_net_bind_service=+ep' /opt/yandex/dataproc-manager/bin/dataproc-manager"])

        if SyncCodeComponents.DataprocUIProxy in sync_components:
            sync_dataproc_ui_proxy(addr, ya_cmd)

    if SyncCodeComponents.DbaasInternalAPI in sync_components:
        _rebuild_app('internal-api', f'{os.getcwd()}/../dbaas-internal-api-image/', ya_cmd)
        rsync_checked(
            '../dbaas-internal-api-image/uwsgi/internal-api.wsgi',
            f'{host}:/opt/yandex/dbaas-internal-api/bin/internal-api.wsgi',
            additional_options=['-L'],
        )
        ssh(addr, ["chmod 755 /opt/yandex/dbaas-internal-api/bin/internal-api.wsgi"])

    if SyncCodeComponents.MdbInternalAPI in sync_components:
        _rebuild_app('mdb-internal-api', f'{os.getcwd()}/../mdb-internal-api/', ya_cmd)
        rsync_checked(
            '../mdb-internal-api/cmd/mdb-internal-api/mdb-internal-api',
            f'{host}:/opt/yandex/mdb-internal-api/mdb-internal-api',
            additional_options=['-L'],
        )
        ssh(addr, ["chmod 755 /opt/yandex/mdb-internal-api/mdb-internal-api"])

    if SyncCodeComponents.DeployAPI in sync_components:
        _rebuild_app('mdb-deploy-api', f'{os.getcwd()}/../deploy/api/cmd/mdb-deploy-api/', ya_cmd)
        rsync_checked(
            '../deploy/api/cmd/mdb-deploy-api/mdb-deploy-api',
            f'{host}:/opt/yandex/mdb-deploy-api/mdb-deploy-api',
            additional_options=['-L'],
        )
        ssh(addr, ["chmod 755 /opt/yandex/mdb-deploy-api/mdb-deploy-api"])

    if not product or product == 'kafka':
        if SyncCodeComponents.KafkaAgent in sync_components:
            sync_kafka_agent(addr, ya_cmd)

        if SyncCodeComponents.AdminApiGateway in sync_components:
            sync_adminapi_gateway(addr, ya_cmd)

    if SyncCodeComponents.Maintenance in sync_components:
        sync_mdb_maintenance(host, addr, ya_cmd)


def sync_dataproc_ui_proxy(fqdn: str, ya_cmd: str):
    _rebuild_app('dataproc-ui-proxy', f'{os.getcwd()}/../dataproc-ui-proxy/', ya_cmd)
    rsync_checked(
        '../dataproc-ui-proxy/cmd/dataproc-ui-proxy/dataproc-ui-proxy',
        f'[{fqdn}]:/opt/yandex/dataproc-ui-proxy/dataproc-ui-proxy',
        additional_options=['-L'],
    )
    ssh(fqdn, ["chmod 755 /opt/yandex/dataproc-ui-proxy/dataproc-ui-proxy"])


def sync_kafka_agent(fqdn: str, ya_cmd: str):
    _rebuild_app('kafka-agent', f'{os.getcwd()}/../kafka_agent/', ya_cmd)
    rsync_checked(
        '../kafka_agent/bin/kafka-agent',
        f'[{fqdn}]:/srv-master/salt/components/kafka/kafka-agent/conf/kafka-agent',
        additional_options=['-L'],
    )


def sync_adminapi_gateway(fqdn: str, ya_cmd: str):
    _rebuild_app('kafka-adminapi-gateway', f'{os.getcwd()}/../packages/external/kafka-adminapi-gateway/', ya_cmd)
    rsync_checked(
        '../packages/external/kafka-adminapi-gateway/external-kafka-adminapi-gateway.jar',
        f'[{fqdn}]:/srv-master/salt/components/kafka/conf/kafka-adminapi-gateway.jar',
        additional_options=['-L'],
    )


def sync_mdb_maintenance(host, addr, ya_cmd):
    _rebuild_app('mdb-maintenance', f'{os.getcwd()}/../mdb-maintenance/', ya_cmd)
    rsync_checked(
        '../mdb-maintenance/cmd/mdb-maintenance-sync/mdb-maintenance-sync',
        f'{host}:/opt/yandex/mdb-maintenance/mdb-maintenance-sync',
        additional_options=['-L'],
    )
    ssh(addr, ["chmod 755 /opt/yandex/mdb-maintenance/mdb-maintenance-sync"])
    rsync_checked('../mdb-maintenance/configs/', f'{host}:/etc/yandex/mdb-maintenance/configs/')


@env_stage('start', fail=True)
def put_ssh_private_key(state: Dict, conf: Dict, **_) -> None:
    """
    Put ssh private key from config to file on control-plane vm
    """
    host = get_driver(state, conf).get_host()
    pair = gen_pair(state, 'dataplane')
    keymap = conf['compute_driver']['ssh_keys'].get('dataplane')
    for name, path in keymap.items():
        code, out, err = remote_file_write(pair[name], host, path, chmod='o+rw,g-rwx,a-rwx', owner='root', group='root')
        if code != 0:
            raise ComputeDriverError(f'Can not write {name} key to {path}, stdout: {out}, stderr: {err}')


def _highstate(state: Dict, conf: Dict, allow_fail=False) -> None:
    """
    Sync control plane code
    """
    driver = get_driver(state, conf)
    addr = driver.get_managed_ipv6_address()

    highstate_cmd = ' '.join(
        [
            'salt-call',
            '--state-out=changes',
            '--output-diff',
            '--out=json',
            '--local',
            '--retcode-passthrough',
            'state.highstate',
            'queue=True',
        ]
    )
    returncode, stdout, stderr = ssh(addr, [highstate_cmd], timeout=1200, stderr=subprocess.DEVNULL)
    try:
        ts = datetime.strftime(datetime.now(), '%Y-%M-%d_%H:%M:%S')
        with open(f'staging/logs/hs_{ts}.out', 'wb') as f:
            f.write(stdout or b'')
        with open(f'staging/logs/hs_{ts}.err', 'wb') as f:
            f.write(stderr or b'')
    except Exception:
        LOG.exception("Failed to write highstate")
    if returncode:
        try:
            states = json.loads(stdout)['local']
            failed_states = ""
            # Sometimes, salt-highstates fails without states and with list of errors
            # Just print then, if we hasn't any failed states.
            if isinstance(states, list):
                raise ComputeDriverError('\n'.join(states))
            for name, salt_state in states.items():
                if not salt_state.get('result', True):
                    desc = json.dumps(salt_state, sort_keys=True, indent=2, separators=(',', ': '))
                    failed_states += f"{name}: {desc}\n"
            if not allow_fail:
                if failed_states == "":
                    raise ValueError("Empty failed states")
                raise ComputeDriverError(f"Failed highstate, states: {failed_states}")
        except ValueError as exc:
            if not allow_fail:
                raise ComputeDriverError("Failed highstate") from exc


def _setcap_gointapi(state: Dict, conf: Dict, **_) -> None:
    """
    Restart redis
    """
    driver = get_driver(state, conf)
    addr = driver.get_managed_ipv6_address()
    ssh_checked(addr, ["setcap 'cap_net_bind_service=+ep' /opt/yandex/mdb-internal-api/mdb-internal-api"])


@env_stage('start', fail=True)
def highstate(state: Dict, conf: Dict, **_) -> None:
    """
    Run highstates with crutches
    """
    # First highstate could fail
    _highstate(state, conf, allow_fail=True)
    _remove_populate_tables(state, conf)
    _setcap_gointapi(state, conf)
    # Second highstate should be successful
    _highstate(state, conf, allow_fail=False)


@env_stage('start', fail=True)
def daemons_restart(state: Dict, conf: Dict, **_) -> None:
    """
    Restart daemons after syncing code
    """
    driver = get_driver(state, conf)
    addr = driver.get_managed_ipv6_address()
    # Sometimes pgbouncer doesn't load certificates correctly, so we should restart him
    ssh_checked(addr, ['service pgbouncer restart'])

    # explicitly stop dbaas-worker
    ssh(addr, ['pkill -9 dbaas-worker'])
    ssh_checked(addr, ['supervisorctl update'], timeout=600)
    ssh_checked(addr, ['supervisorctl restart all'], timeout=600)

    _setcap_gointapi(state, conf)
    ssh_checked(addr, ['service mdb-internal-api restart'])
    ssh_checked(addr, ['service nginx restart'])

    ssh_checked(addr, ['service dataproc-ui-proxy restart'])


@env_stage('start', fail=True)
def daemons_restart_managed(state: Dict, conf: Dict, **_) -> None:
    daemons_restart(state, conf)
    _setcap_gointapi(state, conf)
    driver = get_driver(state, conf)
    addr = driver.get_managed_ipv6_address()
    units = [
        'salt-api.service',
        'salt-master.service',
        'mdb-deploy-api.service',
        'mdb-deploy-saltkeys.service',
        'mdb-ping-salt-master.service',
        'mdb-internal-api.service',
        'mdb-deploy-api.service',
    ]
    for unit in units:
        ssh_checked(addr, ['systemctl restart ', unit])


def cleanup_environment(context):
    """
    Cleanup metadb and compute environment
    """
    LOG.info('Cleaning environment ...')
    driver = get_driver(context.state, context.conf)
    hosts = get_all_hosts(context)
    driver.delete_instances(hosts)
    truncate_clusters_cascade(context)
    truncate_worker_queue_cascade(context)


@env_stage('clean', fail=True)
def cleanup_environment_step(state: Dict, conf: Dict, **_) -> None:
    """
    Cleanup metadb and compute environment (step)
    """
    context = FakeContext(state, conf)
    try:
        cleanup_environment(context)
    except Exception:
        # We can't connect to the host if it's absent
        pass


def is_outdated(e: Union[str, datetime], ttl: timedelta = None) -> (bool, timedelta):
    """
    Return is outdated and age of event `e`
    """
    if ttl is None:
        ttl = timedelta(hours=4)
    event, now = None, datetime.now().astimezone(timezone.utc)
    if type(e) == str:
        event = datetime.fromisoformat(e).astimezone(timezone.utc)
    elif type(e) == datetime:
        event = e.astimezone(timezone.utc)
    else:
        raise RuntimeError('is_outdated doesn\'t support {type(o)}, for value {o}')
    age = now - event
    return age >= ttl, age


@env_stage('gc_infratest_vms', fail=False)
def gc_infratest_vms(state: Dict, conf: Dict, **_) -> None:
    """
    Clean `my` vm older than 4 hours
    """
    dry_run = False
    if os.environ.get('GC_DRY_RUN') is not None:
        dry_run = True

    compute_api = ComputeApi(conf['compute'], LOG)
    folder_id = conf['compute_driver']['dataplane_folder_id']
    all_instances = compute_api.list_instances(folder_id)

    operations = []
    for instance in all_instances:
        labels = instance.labels
        owner = labels.get('owner')
        cluster_id = labels.get('cluster_id')
        is_ci_instance = owner == 'robot-pgaas-ci'
        is_user_data_plane_instance = owner == conf['user'] and cluster_id
        owner_not_defined = owner is None
        if is_ci_instance or is_user_data_plane_instance or owner_not_defined:
            outdated, age = is_outdated(instance.created_at.ToDatetime().replace(tzinfo=timezone.utc))
            if not outdated:
                continue
            fqdn = instance.fqdn
            if owner_not_defined:
                LOG.info(f'Deleting instance {fqdn}, age: {age}, cid: {cluster_id}')
            else:
                LOG.info(f'Deleting instance {fqdn}, age: {age}, cid: {cluster_id}, owner: {owner}')
            if dry_run:
                continue

            operation = compute_api.instance_absent(
                fqdn,
                instance_id=instance.id,
                folder_id=folder_id,
            )
            if operation:
                operations.append(operation)
    for operation in operations:
        compute_api.compute_operation_wait(operation)


@env_stage('gc_infratest_vms', fail=False)
def gc_infratest_instance_groups(state: Dict, conf: Dict, **_) -> None:
    """
    Clean `my` vm older than 4 hours
    """
    dry_run = False
    if os.environ.get('GC_DRY_RUN') is not None:
        dry_run = True

    instance_group_api = InstanceGroupApi(conf['compute'], LOG)
    folder_id = conf['compute_driver']['dataplane_folder_id']
    all_instance_groups = instance_group_api.list(folder_id).data.instance_groups
    operations = []
    for instance_group in all_instance_groups:
        name = instance_group.name
        if 'dataproc' in name:
            labels = None
            try:
                user_metadata = yaml.safe_load(instance_group.instance_template.metadata['user-data'])
                subcluster_labels = user_metadata['data']['labels']
                # get labels of the first subcluster -- they are the same anyway
                labels = next(iter(subcluster_labels.values()))
            except (TypeError, KeyError):
                pass
            if labels:
                owner = labels.get('owner')
                cluster_id = labels.get('cluster_id')
                if owner == conf['user'] and cluster_id:
                    outdated, age = is_outdated(instance_group.created_at.ToDatetime().replace(tzinfo=timezone.utc))
                    if not outdated:
                        continue
                    LOG.info(
                        f'Deleting instance group {name}, id: {instance_group.id},'
                        f'age: {age}, cid: {cluster_id}, owner: {owner}'
                    )
                    if dry_run:
                        continue
                    references = instance_group_api.list_references(instance_group_id=instance_group.id).data.references
                    referrer_id = None
                    if references:
                        referrer_id = references[0].referrer.id
                    operation = instance_group_api.delete(
                        instance_group_id=instance_group.id,
                        referrer_id=referrer_id,
                    )
                    if operation:
                        operations.append(operation)
    for operation in operations:
        instance_group_api.wait(operation)


@env_stage('list_infratest_vms', fail=False)
def list_infratest_vms(state: Dict, conf: Dict, **_) -> None:
    """
    List all VM's according to infratests
    """
    compute_api = ComputeApi(conf['compute'], LOG)
    all_instances = compute_api.list_instances(conf['compute_driver']['dataplane_folder_id'])
    instances_by_owner = {}
    for instance in all_instances:
        labels = instance.labels
        owner = labels.get('owner', 'unknown')
        instances = instances_by_owner.get(owner, [])
        instances.append(instance)
        instances_by_owner[owner] = instances

    for owner, owner_instances in instances_by_owner.items():
        print(f"{owner}:")
        instances_by_cid = {}

        for instance in owner_instances:
            cid = instance.labels.get('cluster_id')
            instances = instances_by_cid.get(cid, [])
            instances.append(instance)
            instances_by_cid[cid] = instances

        for cid, cluster_instances in instances_by_cid.items():
            print(f"    cid = {cid}:")
            for instance in cluster_instances:
                name = instance.name or instance.id
                created = instance.created_at.ToDatetime().replace(tzinfo=timezone.utc)
                print(f"        {name}, id = {instance.id}, created = {created}")


@env_stage('gc_infratest_snapshots', fail=False)
def gc_infratest_snapshots(state: Dict, conf: Dict, **_) -> None:
    """
    Delete old snapshots
    """
    dry_run = False
    if os.environ.get('GC_DRY_RUN') is not None:
        dry_run = True

    compute_api = ComputeApi(conf['compute'], LOG)
    folder_id = conf['compute_driver']['dataplane_folder_id']
    all_snapshots = compute_api.list_snapshots(folder_id)
    operation_ids = []
    for snapshot in all_snapshots:
        labels = snapshot.labels
        keep_until = labels.get('keep_until')
        if keep_until:
            delete = False
            try:
                keep_until = int(keep_until)
                keep_until = datetime.fromtimestamp(keep_until)
                delete = keep_until < datetime.now()
            except ValueError:
                pass

            LOG.info(f"Found dataproc snapshot {snapshot.id}, expired: {delete}")
            if delete:
                LOG.info(f"Deleting snapshot {snapshot.id}")
                if not dry_run:
                    operation_id = compute_api.delete_snapshot(snapshot.id)
                    operation_ids.append(operation_id)
    for operation_id in operation_ids:
        compute_api.compute_operation_wait(operation_id)


@env_stage('gc_infratest_s3', fail=False)
def gc_infratest_s3(state: Dict, conf: Dict, **_) -> None:
    """
    Clean backups from S3 older than 4 hours
    """
    dry_run = False
    if os.environ.get('GC_DRY_RUN') is not None:
        dry_run = True

    bucket_prefix = mdb_internal_api_pillar(conf)['config']['logic']['s3_bucket_prefix']
    if len(bucket_prefix) < 10:
        LOG.error(f'Bucket prefix "{bucket_prefix}" is too short. It is dangerous to proceed.')
        return

    s3 = conf['s3']
    all_buckets, owner = list_buckets(s3)

    for bucket in all_buckets:
        LOG.debug(f'Found    bucket {bucket["Name"]}, owner {owner["ID"]}')
        if bucket['Name'].startswith(bucket_prefix):
            outdated, age = is_outdated(bucket['CreationDate'], timedelta(hours=4))
            LOG.debug(f'Found    bucket {bucket["Name"]}, owner {owner["ID"]} age: {age}')
            if not outdated:
                continue
            LOG.info(f'Deleting bucket {bucket["Name"]}, owner {owner["ID"]} age: {age}')
            if dry_run:
                continue
            delete_bucket(s3, bucket["Name"])


def get_security_group_ids_in_alive_vms(conf: Dict):
    """
    :return: ids of security-groups related to alive instances in specified folder.
    """
    compute_api = ComputeApi(conf['compute'], LOG)
    folder_id = conf['compute_driver']['dataplane_folder_id']
    instances = compute_api.list_instances(folder_id)
    result = set()
    for instance in instances:
        networks = instance.network_interfaces
        for network in networks:
            security_group_ids_in_network = network.security_group_ids
            for sg_id in security_group_ids_in_network:
                result.add(sg_id)
    return result


@env_stage('gc_infratest_security_groups', fail=False)
def gc_infratest_security_groups(state: Dict, conf: Dict, **_) -> None:
    """
    Clean old security_groups in specified folder.
    """
    dry_run = False
    if os.environ.get('GC_DRY_RUN') is not None:
        dry_run = True

    compute_api = ComputeApi(conf['compute'], LOG)
    folder_id = conf['compute_driver']['dataplane_folder_id']
    all_security_groups = compute_api.list_security_groups(folder_id)
    security_groups_ids_in_alive_vms = get_security_group_ids_in_alive_vms(conf)
    operation_ids = []
    for security_group in all_security_groups:
        _, age = is_outdated(security_group.created_at.ToDatetime().replace(tzinfo=timezone.utc))
        is_service_cid_mdb = 'service_cid_mdb' in security_group.name
        is_in_alive_vm = security_group.id in security_groups_ids_in_alive_vms
        if not is_service_cid_mdb or security_group.default_for_network or is_in_alive_vm:
            continue
        LOG.info(
            f'Deleting security group {security_group.name}, id: {security_group.id},'
            f'age: {age}, folder_id: {security_group.folder_id}, network_id: {security_group.network_id}'
        )
        if dry_run:
            continue
        operation_id = compute_api.delete_security_group(security_group.id)
        operation_ids.append(operation_id)
    for operation_id in operation_ids:
        compute_api.vpc_operation_wait(operation_id)
