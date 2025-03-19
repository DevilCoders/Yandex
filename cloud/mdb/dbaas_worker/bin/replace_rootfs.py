"""
Script for rootfs disk replace in compute and porto
"""

import logging
import time
from queue import Queue
from types import SimpleNamespace
from typing import List

from copy import deepcopy

from cloud.mdb.dbaas_worker.internal.config import get_config, worker_args_parser
from cloud.mdb.dbaas_worker.internal.logs import get_task_prefixed_logger
from cloud.mdb.dbaas_worker.internal.providers.base_metadb import BaseMetaDBProvider
from cloud.mdb.dbaas_worker.internal.providers.compute import ComputeApi
from cloud.mdb.dbaas_worker.internal.providers.dbm import DBMApi
from cloud.mdb.dbaas_worker.internal.providers.deploy import (
    DeployAPI,
    DeployShipmentCommand,
    DeployShipmentData,
    deploy_dataplane_host,
)
from cloud.mdb.dbaas_worker.internal.providers.dns import DnsApi, Record
from cloud.mdb.dbaas_worker.internal.providers.mlock import Mlock
from cloud.mdb.dbaas_worker.internal.providers.tls_cert import TLSCert
from cloud.mdb.dbaas_worker.internal.query import execute
from cloud.mdb.dbaas_worker.internal.runners import get_host_opts
from cloud.mdb.internal.python.compute.images import ImageModel
from cloud.mdb.internal.python.compute.instances import InstanceModel

MD_FIX_CMD = r"""
if ls /dev/md* >/dev/null 2>&1;
then
    for i in /dev/md*;
    do
        mdadm --stop $i;
    done;
    echo 'CREATE owner=root group=disk mode=0660 auto=yes' > /etc/mdadm/mdadm.conf;
    echo 'HOMEHOST <system>' >> /etc/mdadm/mdadm.conf;
    echo 'MAILADDR root' >> /etc/mdadm/mdadm.conf;
    echo "$(/sbin/mdadm --examine --scan | /bin/sed 's/\/dev\/md\/0/\/dev\/md0/g')" >> /etc/mdadm/mdadm.conf;
    mdadm --assemble --scan;
    /usr/sbin/update-initramfs -u -k all;
fi
"""

IMAGE_ROLE_MAP = {
    'zk': 'zookeeper',
    'mongodb_cluster.mongocfg': 'mongodb',
    'mysql_cluster': 'mysql',
    'sqlserver_cluster': 'sqlserver',
    'redis_cluster': 'redis',
    'clickhouse_cluster': 'clickhouse',
    'mongodb_cluster.mongos': 'mongodb',
    'postgresql_cluster': 'postgresql',
    'mongodb_cluster.mongod': 'mongodb',
    'kafka_cluster': 'kafka',
}

PROPERTIES_ROLE_MAP = {
    'zk': 'zookeeper',
    'clickhouse_cluster': 'clickhouse',
    'mongodb_cluster.mongocfg': 'mongocfg',
    'mongodb_cluster.mongos': 'mongos',
    'mongodb_cluster.mongod': 'mongod',
    'mongodb_cluster.mongoinfra': 'mongoinfra',
    'mysql_cluster': 'mysql',
    'sqlserver_cluster': 'sqlserver',
    'redis_cluster': 'redis',
    'postgresql_cluster': 'postgresql',
    'kafka_cluster': 'kafka',
}


class RootfsReplacer:
    """
    Provider for rootfs disk replace in compute
    """

    def __init__(self, config, task, queue):
        self.dns_api = DnsApi(config, task, queue)
        self.metadb = BaseMetaDBProvider(config, task, queue)
        self.deploy_api = DeployAPI(config, task, queue)
        self.mlock = Mlock(config, task, queue)
        self.dbm = DBMApi(config, task, queue)
        self.logger = get_task_prefixed_logger(task, __name__)
        self.compute = ComputeApi(config, task, queue)
        self.config = config
        self.task = task
        self.queue = queue

    def create_new_boot(self, instance: InstanceModel, image: ImageModel, preserve_size, type_id='network-hdd'):
        """
        Create new boot disk from image
        """
        if preserve_size:
            size = self.compute._get_disk(instance.boot_disk.disk_id).size
        else:
            size = image.min_disk_size
        fqdn = instance.fqdn
        operation_id, disk_id = self.compute.create_disk(
            geo=instance.zone_id,
            size=size,
            type_id=type_id,
            image_id=image.id,
            context_key=f'create-disk-for-{fqdn}-replace-rootfs',
        )
        self.compute.operation_wait(operation_id)
        return disk_id

    def update_boot_disk(self, instance, new_boot_disk_id):
        """
        Update instance boot disk
        """
        operation_id = self.compute.update_instance_boot_disk(
            instance.id,
            boot_disk_id=new_boot_disk_id,
        )
        self.compute.operation_wait(operation_id)

    def start_instance(self, instance):
        """
        Start instance
        """
        operation_id = self.compute.instance_running(instance.fqdn, instance.id)
        if operation_id:
            self.compute.operation_wait(operation_id)

    def stop_instance(self, instance, context_suffix='stop'):
        """
        Stop instance
        """
        _, properties = self.get_opts_and_properties(instance.fqdn)
        operation_id = self.compute.instance_stopped(
            instance.fqdn,
            instance.id,
            context_suffix=context_suffix,
            termination_grace_period=getattr(properties, 'termination_grace_period', None),
        )
        if operation_id:
            self.compute.operation_wait(operation_id)

    def get_host_info(self, fqdn):
        """
        Get host info (cluster_id, cluster_type, instance_id)
        """
        with self.metadb.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                res = execute(cur, 'get_host_info', fqdn=fqdn)
                return res[0]

    def get_base_task_args(self, cluster_id):
        """
        Get common task args (e.g. hosts)
        """
        args = {'hosts': {}}
        with self.metadb.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                res = execute(cur, 'generic_resolve', cid=cluster_id)
        for row in res:
            args['hosts'][row['fqdn']] = get_host_opts(row)

        return args

    def get_image_type(self, image_type, fqdn):
        """
        Discover image type
        """
        if not image_type:
            info = self.get_host_info(fqdn)
            args = self.get_base_task_args(info['cid'])
            return IMAGE_ROLE_MAP[args['hosts'][fqdn]['roles'][0]]
        return image_type

    def get_properties(self, image_type):
        """
        Returns properties by image_type, taking templates into account
        """
        properties = getattr(self.config, image_type, None)
        if properties is not None:
            return properties
        for k in dir(self.config):
            properties = getattr(self.config, k, SimpleNamespace())
            it_template = getattr(properties, 'compute_image_type_template', None)
            if it_template is None:
                continue
            template = it_template['template']
            arg = it_template['task_arg']
            whitelist = it_template['whitelist']
            for v in whitelist.values():
                if image_type == template.format(**{arg: v}):
                    return properties
        return SimpleNamespace()

    def get_opts_and_properties(self, fqdn):
        """
        Discover properties
        """
        info = self.get_host_info(fqdn)
        args = self.get_base_task_args(info['cid'])
        opts = args['hosts'][fqdn]
        opts['cid'] = info['cid']
        attribute = PROPERTIES_ROLE_MAP[args['hosts'][fqdn]['roles'][0]]
        return opts, getattr(self.config, attribute)

    def get_bootstrap_cmd(self, bootstrap_cmd, fqdn):
        """
        Discover bootstrap_cmd
        """
        if not bootstrap_cmd:
            _, properties = self.get_opts_and_properties(fqdn)
            return properties.dbm_bootstrap_cmd

        return bootstrap_cmd

    def update_cert(self, fqdn, update_flag):
        """
        Force update tls cert
        """
        if update_flag:
            opts, properties = self.get_opts_and_properties(fqdn)
            if properties.issue_tls:
                alt_names = [f'{fqdn.split(".")[0]}.{properties.managed_zone}']
                for dns_group in properties.group_dns:
                    id_key = dns_group['id']
                    if id_key == 'cid':
                        group_name = dns_group['pattern'].format(id=opts['cid'])
                    else:
                        group_name = dns_group['pattern'].format(id=opts[id_key], cid=opts['cid'])
                    alt_names.append(group_name)
                task = deepcopy(self.task)
                task['cid'] = opts['cid']
                tls_cert = TLSCert(self.config, task, self.queue)
                tls_cert.exists(fqdn, alt_names=alt_names, force=True)

    def get_secrets(self):
        """
        Get secrets configuration
        """
        return {
            '/etc/yandex/mdb-deploy/deploy_version': {
                'mode': '0644',
                'content': '2',
            },
            '/etc/yandex/mdb-deploy/mdb_deploy_api_host': {
                'mode': '0644',
                'content': deploy_dataplane_host(self.config),
            },
        }

    def fix_deploy(self, fqdn):
        """
        Unregister/create minion in deploy api
        """
        if self.deploy_api.has_minion(fqdn):
            self.deploy_api.unregister_minion(fqdn)
        else:
            self.deploy_api.create_minion(fqdn, self.config.deploy.group)

    def prepare_porto_container(self, fqdn, bootstrap_cmd):
        """
        Run container with new secrets and bootstrap cmd
        """
        secrets = self.get_secrets()

        change = self.dbm.update_container(fqdn, data=dict(secrets=secrets, bootstrap_cmd=bootstrap_cmd))

        if change.jid is None and change.operation_id is None:
            raise RuntimeError(f'Unable to init secrets deploy. DBM change: {change}')

        if change.operation_id:
            self.dbm.wait_operation(change.operation_id)
        else:
            self.deploy_api.wait([change.jid])

    def deploy_compute_windows(self, args):
        """
        Deploy hosts in windows cluster
        """
        info = self.get_host_info(args.host)
        hosts = self.get_base_task_args(info['cid'])['hosts']
        hs_args = []
        if args.args:
            hs_args = [args.args]
        elif len(hosts) > 1:
            hs_args = ['pillar={"replica": true}']
        deploys = []
        for host, opts in hosts.items():
            if host == args.host:
                cmd = {
                    'type': 'state.highstate',
                    'arguments': ["queue=True"] + hs_args,
                    'timeout': self.config.deploy.timeout,
                }
                title = f'highstate-{host}'
            else:
                cmd = {
                    'type': 'state.sls',
                    'arguments': [
                        'components.dbaas-operations.metadata',
                        'concurrent=True',
                        'saltenv={environment}'.format(environment=opts['environment']),
                        'sync_mods=states,modules',
                    ],
                    'timeout': (self.config.deploy.timeout // self.deploy_api.default_max_attempts),
                }
                title = f'metadata-skip-{args.host}'
            deploys.append(
                self.deploy_api.run(
                    host,
                    method={
                        'commands': [cmd],
                        'fqdns': [host],
                        'parallel': 1,
                        'stopOnErrorCount': 1,
                        'timeout': self.config.deploy.timeout,
                    },
                    deploy_title=title,
                )
            )
        self.deploy_api.wait(deploys)

    def _exec_shipment(self, shipment_type: str, fqdn: str, arguments: List[str], timeout: int = 600):
        command = DeployShipmentCommand(
            arguments=arguments,
            timeout=timeout,
            type=shipment_type,
        )
        shipment_data = DeployShipmentData(
            commands=[command],
            fqdns=[fqdn],
            timeout=timeout,
            parallel=1,
            stopOnErrorCount=1,
        )
        jids = self.deploy_api.create_shipment(shipment_data)
        self.deploy_api.wait(jids)

    def _exec_command(self, fqdn: str, cmd: str):
        self._exec_shipment('cmd.run', fqdn, [cmd])

    def deploy_compute_linux(self, args):
        """
        Deploy linux hosts in compute
        """
        fqdn = args.host
        if args.preserve:
            self._exec_command(fqdn, 'echo ",+," | sfdisk --force -N1 /dev/vda || /bin/true')
            self._exec_command(fqdn, 'partprobe /dev/vda')
            self._exec_command(fqdn, 'resize2fs /dev/vda1')
        self._exec_command(fqdn, MD_FIX_CMD)
        self._exec_shipment('saltutil.sync_all', fqdn, [])
        hs_args = ['queue=True']
        if args.args:
            hs_args.append(args.args)
        self._exec_shipment('state.highstate', fqdn, hs_args)

    def replace_compute(self, args):
        """
        Compute-specific replace logic
        """
        image_type = self.get_image_type(args.image, args.host)
        properties = self.get_properties(image_type)
        instance = self.compute.get_instance(args.host)
        if not instance:
            raise RuntimeError(f'Unable to find instance {args.host}')
        image = self.compute._get_latest_image(image_type)
        disk_type_id = getattr(properties, 'compute_root_disk_type_id', None)
        create_disk_kwargs = {}
        if disk_type_id is not None:
            create_disk_kwargs['type_id'] = disk_type_id
        new_boot_disk_id = self.create_new_boot(instance, image, args.preserve, **create_disk_kwargs)
        self.logger.info('New disk: %s', new_boot_disk_id)
        self.stop_instance(instance)
        if args.offline:
            self.compute.operation_wait(self.compute.disable_billing(args.host))
        self.update_boot_disk(instance, new_boot_disk_id)
        self.fix_deploy(args.host)
        self.start_instance(instance)
        self.update_cert(args.host, args.tls)
        self.deploy_api.wait_minions_registered(args.host)
        if getattr(properties, 'host_os', None) == 'windows':
            self.deploy_compute_windows(args)
        else:
            self.deploy_compute_linux(args)
        if args.offline:
            self.stop_instance(instance, context_suffix='stop2')
            self.compute.operation_wait(self.compute.enable_billing(args.host))
        elif not args.skip_dns:
            setup_address = self.compute.get_instance_setup_address(args.host)
            managed_records = [
                Record(
                    address=setup_address,
                    record_type='AAAA',
                ),
            ]
            self.dns_api.set_records(
                f'{args.host.split(".")[0]}.{self.config.postgresql.managed_zone}', managed_records
            )
            public_records = []
            for address, version in self.compute.get_instance_public_addresses(args.host):
                public_records.append(Record(address=address, record_type=('AAAA' if version == 6 else 'A')))
            self.dns_api.set_records(args.host, public_records)

    def replace_porto(self, args):
        """
        Porto-specific replace logic
        """

        host = args.host
        bootstrap_cmd = self.get_bootstrap_cmd(args.bootstrap_cmd, host)
        container = self.dbm.get_container(host)
        if not container:
            raise RuntimeError(f'Unable to find container {host}')

        volumes = self.dbm.get_volumes(host)
        dom0 = container['dom0']

        self._exec_command(dom0, f'portoctl destroy {host} || /bin/true')
        timestamp = int(time.time())
        self._exec_command(
            dom0, f'mv {volumes["/"]["dom0_path"]} {volumes["/"]["dom0_path"]}.{timestamp}.bak || /bin/true'
        )
        self.fix_deploy(host)
        self.prepare_porto_container(host, bootstrap_cmd)
        self._exec_shipment('saltutil.sync_all', host, [])

        hs_args = ['queue=True']
        if args.args:
            hs_args.append(args.args)
        self._exec_shipment('state.highstate', host, hs_args)

    def replace_rootfs(self, args):
        """
        Common entry-point
        """
        try:
            self.mlock.lock_cluster(hosts=[args.host])
            if args.vtype == 'compute':
                self.replace_compute(args)
            else:
                self.replace_porto(args)
        finally:
            self.mlock.unlock_cluster()


def do_replace_rootfs():
    """
    Console entry-point
    """
    parser = worker_args_parser(
        description="Example: ./replace-rootfs -v compute rc1c-jgq5w8e1a23o0z7v.mdb.cloud-preprod.yandex.net",
    )
    parser.add_argument('-a', '--args', type=str, help='Pass additional args to highstate call')
    parser.add_argument(
        '-v',
        '--vtype',
        default='compute',
        const='compute',
        nargs='?',
        choices=['compute', 'porto'],
        help='Target vtype (compute or porto, default: %(default)s)',
    )
    parser.add_argument('-o', '--offline', action='store_true', help='Do offline replace (compute only)')
    parser.add_argument('-t', '--tls', action='store_true', help='Update TLS cert')
    parser.add_argument('-i', '--image', type=str, help='Image type override')
    parser.add_argument('-b', '--bootstrap_cmd', type=str, help='DBM bootstrap command')
    parser.add_argument('-p', '--preserve', action='store_true', help='Preserve original disk size')
    parser.add_argument('-s', '--skip-dns', action='store_true', help='Do not touch dns')
    parser.add_argument('host', type=str, help='target host FQDN')
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s:\t%(message)s')
    config = get_config(args.config)
    replacer = RootfsReplacer(
        config,
        {
            'cid': f'rootfs-replace-{args.host}',  # used for mlock lock id
            'task_id': 'rootfs-replace',
            'timeout': 24 * 3600,
            'changes': [],
            'context': {},
            'feature_flags': [],
            'folder_id': 'test',
        },
        Queue(maxsize=10**6),
    )

    if args.vtype == 'compute' and args.host.endswith(config.postgresql.managed_zone):
        raise RuntimeError(f'Managed hostname {args.host} used')
    replacer.replace_rootfs(args)
