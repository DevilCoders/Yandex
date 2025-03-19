"""
Common create executor
"""
from types import SimpleNamespace
from typing import Callable, List

from ...cloud_init import cloud_init_user_data
from ...crypto import encrypt
from ...providers.aws.byoa import BYOA
from ...providers.aws.ec2 import EC2, EC2DiscSpec, EC2InstanceSpec
from ...providers.aws.iam import AWSIAM, DEFAULT_EC2_ASSUME_ROLE_POLICY, AWSRole
from ...providers.aws.kms import KMS
from ...providers.aws.route53 import Route53
from ...providers.compute import ComputeApi, get_core_fraction
from ...providers.conductor import ConductorApi
from ...providers.dbm import DBMApi
from ...providers.deploy import deploy_dataplane_host
from ...providers.disk_placement_group import DiskPlacementGroupProvider, DiskType
from ...providers.dns import DnsApi, Record
from ...providers.eds import EdsApi
from ...providers.gpg_backup_key import GpgBackupKey
from ...providers.iam import Iam
from ...providers.internal_api import InternalApi
from ...providers.juggler import JugglerApi
from ...providers.mdb_vpc import VPC
from ...providers.metadb_backup_service import MetadbBackups
from ...providers.pillar import DbaasPillar
from ...providers.placement_group import PlacementGroupProvider
from ...providers.resource_manager import ResourceManagerApi
from ...providers.s3_bucket import S3Bucket
from ...providers.s3_bucket_access import S3BucketAccess
from ...providers.solomon_client import SolomonApiV2
from ...providers.tls_cert import TLSCert
from ...providers.vpc import VPCProvider
from ...types import HostGroup
from ...utils import get_conductor_root_group, get_eds_root, get_first_value, get_image_by_major_version, get_paths
from ..utils import (
    build_host_group,
    get_cluster_encryption_key_alias,
    get_managed_hostname,
    get_private_hostname,
    issue_tls_wrapper,
)
from .deploy import BaseDeployExecutor

_WaitUntilDone = Callable[[], None]


# pylint: disable=too-many-instance-attributes
class BaseCreateExecutor(BaseDeployExecutor):
    """
    Base class for create executors
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.compute_api = ComputeApi(self.config, self.task, self.queue)
        self.ec2 = EC2(self.config, self.task, self.queue)
        self.route53 = Route53(self.config, self.task, self.queue)
        self.disk_placement = DiskPlacementGroupProvider(self.config, self.task, self.queue)
        self.dbm_api = DBMApi(self.config, self.task, self.queue)
        self.dns_api = DnsApi(self.config, self.task, self.queue)
        self.juggler = JugglerApi(self.config, self.task, self.queue)
        self.conductor = ConductorApi(self.config, self.task, self.queue)
        self.eds = EdsApi(self.config, self.task, self.queue)
        self.solomon = SolomonApiV2(self.config, self.task, self.queue)
        self.s3_bucket = S3Bucket(self.config, self.task, self.queue)
        self.tls_cert = TLSCert(self.config, self.task, self.queue)
        self.gpg_backup_key = GpgBackupKey(self.config, self.task, self.queue)
        self.internal_api = InternalApi(self.config, self.task, self.queue)
        self.resource_manager = ResourceManagerApi(self.config, self.task, self.queue)
        self.mdb_vpc = VPC(self.config, self.task, self.queue)
        self.iam = Iam(config, task, queue)
        self.pillar = DbaasPillar(config, task, queue)
        self.backup_db = MetadbBackups(self.config, self.task, self.queue)
        self.placement_group = PlacementGroupProvider(self.config, self.task, self.queue)
        self.s3_bucket_access = S3BucketAccess(self.config, self.task, self.queue)
        self.vpc = VPCProvider(self.config, self.task, self.queue)
        self.kms = KMS(self.config, self.task, self.queue)
        self.aws_iam = AWSIAM(self.config, self.task, self.queue)
        self.byoa = BYOA(self.config, self.task, self.queue)

    def _create_conductor_group(self, host_group):
        """
        Create conductor group for host group
        """
        self.conductor.group_exists(
            getattr(host_group.properties, 'conductor_group_id', self.task['cid']),
            get_conductor_root_group(host_group.properties, get_first_value(host_group.hosts)),
        )

    def _create_s3_bucket(self):
        """
        Create s3 bucket for clusters backups
        """
        if self.args.get('s3_buckets'):
            for endpoint, bucket in self.args['s3_buckets'].items():
                self.s3_bucket.exists(bucket, endpoint)
                self.s3_bucket_access.creds_exist(endpoint)
                self.s3_bucket_access.grant_access(bucket, 'rw', endpoint)
            return

        # TODO: Remove after int-api and worker will both work with s3_buckets task argument.
        if self.args.get('s3_bucket'):
            self.s3_bucket.exists(self.args['s3_bucket'])

    def _create_host_secrets(self, host_group):
        """
        Create salt secrets for hosts
        """
        for host, opts in host_group.hosts.items():
            opts['secrets'] = {}

            paths = get_paths(host_group.properties.host_os)

            opts['secrets'].update(
                {
                    paths.DEPLOY_VERSION: {
                        'mode': '0644',
                        'content': str(self.config.deploy.version),
                    },
                    paths.DEPLOY_API_HOST: {
                        'mode': '0644',
                        'content': deploy_dataplane_host(self.config),
                    },
                }
            )

    def _render_user_data(self, host):
        return cloud_init_user_data(
            fqdn=host,
            deploy_version=str(self.config.deploy.version),
            mdb_deploy_api_host=deploy_dataplane_host(self.config),
        )

    def _join_labels(self, host_group, opts, extra_labels=None):
        labels = {}
        labels.update(getattr(host_group.properties, 'labels', {}))
        labels.update(self.args.get('labels', {}))
        labels.update(opts.get('labels', {}))
        if extra_labels:
            labels.update(extra_labels)
        return labels

    def _create_compute_vm(self, host, opts, host_group, revertable) -> _WaitUntilDone:
        metadata = {
            'user-data': self._render_user_data(host),
        }
        if getattr(host_group.properties, 'enable_oslogin', False):
            metadata['enable-oslogin'] = 'true'
        if self.args.get('create_offline'):
            metadata['billing-disabled'] = 'true'
        placement_info = self.disk_placement.get_placement_info(
            DiskType(opts['disk_type_id']),
            folder_id=self.config.compute.folder_id,
            geo=opts['geo'],
            cid=self.task['cid'],
            subcid=opts['subcid'],
            shard_id=opts['shard_id'],
            pg_num=opts.get('pg_num', None),
        )
        placement_group_info = self.placement_group.get_placement_group_info(
            folder_id=self.config.compute.folder_id,
            cid=self.task['cid'],
            shard_id=opts['shard_id'],
            subcid=opts['subcid'],
            local_id=opts.get('pg_local_id', None),
            best_effort=self.config.compute.placement_group.get('best_effort', False),
        )

        # Use snake_case for label names here (uppercase is not allowed for label names in compute api)
        extra_labels = {
            'folder_id': self.task['folder_id'],
            'cluster_id': self.task['cid'],
            'cluster_type': opts['cluster_type'],
            'subcluster_type': '/'.join(opts['roles'] or []),
        }
        operation_id = self.compute_api.instance_exists(
            opts['geo'],
            host,
            managed_fqdn=get_managed_hostname(host, opts),
            image_type=get_image_by_major_version(
                image_template=getattr(host_group.properties, 'compute_image_type_template', None),
                image_fallback=host_group.properties.compute_image_type,
                task_args=self.args,
            ),
            platform_id=opts['platform_id'],
            subnet_id=opts['subnet_id'],
            memory=opts['memory_limit'],
            cores=opts['cpu_limit'],
            gpus=opts.get('gpu_limit', 0),
            io_cores=opts.get('io_cores_limit', 0),
            core_fraction=get_core_fraction(opts['cpu_guarantee'], opts['cpu_limit']),
            assign_public_ip=opts['assign_public_ip'],
            security_groups=self.desired_security_groups,
            disk_size=opts['space_limit'],
            root_disk_type_id=host_group.properties.compute_root_disk_type_id,
            disk_type_id=opts['disk_type_id'],
            metadata=metadata,
            service_account_id=opts.get('service_account_id'),
            revertable=revertable,
            labels=self._join_labels(host_group, opts, extra_labels),
            disk_placement_group_id=placement_info.disk_placement_group_id,
            host_group_ids=opts['host_group_ids'],
            vhost_net=opts.get('vhost_net', False),
            placement_group_id=placement_group_info.placement_group_id,
        )

        def wait_for_compute_vm_up_and_running():
            if operation_id:
                if opts['disk_type_id'].startswith('local-'):
                    self.compute_api.operation_wait(operation_id, timeout=50 * 60)
                else:
                    self.compute_api.operation_wait(operation_id)
                self.compute_api.instance_running(host, instance_id=None)
            else:
                self.logger.info('Skipping creating %s', host)
            disk_placement_group_num = opts.get('pg_num', None)
            if disk_placement_group_num is not None:
                self.compute_api.save_disk_placement_group_meta(
                    placement_info.disk_placement_group_id, disk_placement_group_num
                )
                self.compute_api.save_disk_meta(host, disk_placement_group_num, opts['disk_mount_point'])
            pg_local_id = opts.get('pg_local_id', None)
            if pg_local_id is not None:
                self.compute_api.save_placement_group_meta(placement_group_info.placement_group_id, host)
            vtype_id = self.compute_api.save_instance_meta(host)
            if getattr(host_group.properties, 'use_superflow_v22', False):
                self.vpc.set_superflow_v22_flag_on_instance(vtype_id)
            self._create_managed_record(host, opts)

        return wait_for_compute_vm_up_and_running

    def _create_porto_container(self, host, opts, host_group, revertable) -> _WaitUntilDone:
        data_dom0_path = '/data/{fqdn}/data'.format(fqdn=host) if opts['disk_type_id'] == 'local-ssd' else '/disks'
        change = self.dbm_api.container_exists(
            self.task['cid'],
            host,
            opts['geo'],
            options={
                'cpu_limit': opts['cpu_limit'],
                'cpu_guarantee': opts['cpu_guarantee'],
                'memory_limit': opts['memory_limit'],
                'memory_guarantee': opts['memory_guarantee'],
                'net_limit': opts['network_limit'],
                'net_guarantee': opts['network_guarantee'],
                'io_limit': opts['io_limit'],
                'project_id': opts['subnet_id'] or self.config.dbm.project_id,
                'managing_project_id': self.config.dbm.managing_project_id,
            },
            volumes={
                '/': {
                    'space_limit': host_group.properties.rootfs_space_limit,
                    'dom0_path': '/data/{fqdn}/rootfs'.format(fqdn=host),
                },
                host_group.properties.dbm_data_path: {
                    'space_limit': opts['space_limit'],
                    'dom0_path': data_dom0_path,
                },
            },
            bootstrap_cmd=get_image_by_major_version(
                image_template=getattr(host_group.properties, 'dbm_bootstrap_cmd_template', None),
                image_fallback=host_group.properties.dbm_bootstrap_cmd,
                task_args=self.args,
            ),
            platform_id=opts['platform_id'],
            secrets=opts['secrets'],
            revertable=revertable,
        )

        def wait_for_dbm_container():
            if change:
                if change.jid:
                    self.deploy_api.wait([change.jid])
                elif change.operation_id:
                    self.dbm_api.wait_operation(change.operation_id)

        return wait_for_dbm_container

    def _create_aws_vm(self, host, opts, host_group, revertable) -> _WaitUntilDone:
        net = self.mdb_vpc.await_network(self.task['network_id'])

        region_name = opts['region_name']
        extra_labels = {
            'ProjectID': self.task['folder_id'],
            'ClusterID': self.task['cid'],
        }

        cluster_type = opts.get('cluster_type')
        if cluster_type:
            extra_labels['ClusterType'] = cluster_type

        roles = opts.get('roles', [])
        if len(roles) == 1:
            extra_labels['SubclusterType'] = roles[0]

        instance_spec = EC2InstanceSpec(
            name=host,
            boot=EC2DiscSpec(
                type=host_group.properties.aws_root_disk_type,
                size=host_group.properties.rootfs_space_limit,
            ),
            data=EC2DiscSpec(
                type=opts['cloud_provider_disk_type'],
                size=opts['space_limit'],
            ),
            instance_type=opts['cloud_provider_flavor_name'],
            image_type='{arch}-{image_type}'.format(arch=opts['arch'], image_type=host_group.properties.aws_image_type),
            labels=self._join_labels(host_group, opts, extra_labels),
            userdata=self._render_user_data(host),
            subnet_id=opts['subnet_id'],
            sg_id=net.aws_external_resources.security_group_id,
            iam_instance_profile=opts.get('instance_role_id', self.config.ec2.instance_profile_arn),
        )
        instance_id = self.ec2.instance_exists(instance_spec, region_name)

        def wait_for_aws_vm_up_and_running():
            self.ec2.instance_wait_until_running(instance_id, region_name)
            self.ec2.save_instance_meta(instance_id, host)
            # Store vtype_id in opts, with it we don't need resolve name to vtype_id (ORION-338)
            opts['vtype_id'] = instance_id
            self._create_managed_record(host, opts)

        return wait_for_aws_vm_up_and_running

    def _create_host_group(self, host_group, revertable=False):
        """
        Generic method for container/vms creation
        """
        wait_for: List[_WaitUntilDone] = []

        self._desired_security_groups_state([host_group])
        for host, opts in host_group.hosts.items():
            if opts['vtype'] == 'compute':
                wait_for.append(self._create_compute_vm(host, opts, host_group, revertable))
            elif opts['vtype'] == 'porto':
                wait_for.append(self._create_porto_container(host, opts, host_group, revertable))
            elif opts['vtype'] == 'aws':
                wait_for.append(self._create_aws_vm(host, opts, host_group, revertable))
            else:
                raise RuntimeError(f'Unknown vtype: {opts["vtype"]} for host {host}')

            if opts['vtype'] == 'aws':
                self.deploy_api.create_minion_region_aware(host, opts['region_name'])
            else:
                self.deploy_api.create_minion(host, self.config.deploy.group)

        for wait in wait_for:
            wait()

        if host_group.properties.create_gpg:
            self.gpg_backup_key.exists(getattr(host_group.properties, 'gpg_target_id', self.task['cid']))

    def _create_managed_record(self, host, opts):
        """
        Create managed dns records for managed hosts
        """
        if opts['vtype'] == 'compute':
            managed_records = [
                Record(
                    address=self.compute_api.get_instance_setup_address(host),
                    record_type='AAAA',
                ),
            ]
            self.dns_api.set_records(get_managed_hostname(host, opts), managed_records)
        elif opts['vtype'] == 'aws':
            addresses = self.ec2.get_instance_addresses(opts['vtype_id'], opts['region_name'])
            self.route53.set_records_in_public_zone(host, addresses.public_records())
            self.route53.set_records_in_public_zone(get_private_hostname(host, opts), addresses.private_records())

    def _create_public_records(self, host_group):
        """
        Create public dns records for managed hosts
        """
        for host, opts in host_group.hosts.items():
            if opts['vtype'] == 'compute':
                public_records = []
                for address, version in self.compute_api.get_instance_public_addresses(host):
                    public_records.append(Record(address=address, record_type=('AAAA' if version == 6 else 'A')))
                self.dns_api.set_records(host, public_records)

    def _issue_tls(self, host_group, force=False, force_tls_certs=False):
        """
        Create TLS certificate for hosts in cluster
        """
        return issue_tls_wrapper(self.task['cid'], host_group, self.tls_cert, force, force_tls_certs=force_tls_certs)

    def _enable_monitoring(self, host_group):
        """
        Add hosts to conductor, set initial downtimes in juggler and create cluster in solomon
        """
        group_id = getattr(host_group.properties, 'conductor_group_id', self.task['cid'])
        for host, opts in host_group.hosts.items():
            managed_hostname = get_managed_hostname(host, opts)
            self.juggler.downtime_exists(managed_hostname)
            self.conductor.host_exists(managed_hostname, opts['geo'], group_id)
            self.eds.host_register(
                managed_hostname,
                group_id,
                get_eds_root(host_group.properties, get_first_value(host_group.hosts)),
            )

    def _before_highstate(self):
        """
        Actions that can be executed before highstate (e.g. update pillar)
        """
        pass

    def _create_aws_instance_profile(self, host_group: HostGroup):
        self.logger.info("Create AWS instance profile")
        role: AWSRole = self.aws_iam.get_cluster_role()
        self.aws_iam.role_exists(
            role,
            DEFAULT_EC2_ASSUME_ROLE_POLICY,
            self.config.aws_iam.managed_dataplane_policy_arns,
        )

        for host in host_group.hosts.values():
            host['instance_role_id'] = role.instance_profile_arn

    def _create_cluster_service_account(self, host_group: HostGroup):
        if self._is_aws(host_group.hosts):
            self._create_aws_instance_profile(host_group)

    def _create_encryption_stuff(self, host_group: HostGroup):
        if not self.pillar.get('cid', self.task['cid'], ['data', 'encryption', 'enabled']):
            return

        if self._is_aws(host_group.hosts):
            role: AWSRole = self.aws_iam.get_cluster_role()

            # suppose all hosts are going to be created in the same region
            region_name = get_first_value(host_group.hosts)['region_name']
            key_id = get_cluster_encryption_key_alias(host_group, self.task['cid'])
            self.kms.key_exists(key_id, role.arn, region_name)

            self.pillar.exists(
                'cid',
                self.task['cid'],
                ['data', 'encryption', 'key'],
                ['type', 'id'],
                ['aws', key_id],
            )

    def _set_network_parameters(self, host_groups: List[HostGroup]):
        if not any(any(host['vtype'] == 'aws' for host in host_group.hosts.values()) for host_group in host_groups):
            return

        net = self.mdb_vpc.await_network(self.task['network_id'])
        for host_group in host_groups:
            for host, opts in host_group.hosts.items():
                if opts['vtype'] != 'aws':
                    continue

                if not opts.get('subnet_id'):
                    for subnet in net.aws_external_resources.subnets:
                        if subnet.zone_id == opts['geo']:
                            opts['subnet_id'] = subnet.subnet_id
                            self.ec2.save_instance_subnet(subnet.subnet_id, host)
                            break
                if not opts.get('subnet_id'):
                    raise RuntimeError(
                        f"Can not get subnet in region {opts['geo']}, "
                        f"there are only subnets: {net.aws_external_resources.subnets}"
                    )

        self.pillar.exists(
            'cid',
            self.task['cid'],
            ['data', 'access'],
            ['user_net_ipv4_cidrs', 'user_net_ipv6_cidrs'],
            [[net.ipv4_cidr_block], [net.ipv6_cidr_block]],
        )

    def _set_byoa_parameters(self, host_groups: list[HostGroup]) -> None:
        if self._is_aws_host_groups(host_groups):
            net = self.mdb_vpc.await_network(self.task['network_id'])
            if net.aws_external_resources.account_id is not None or net.aws_external_resources.iam_role_arn is not None:
                self.byoa.set_params(
                    iam_role=net.aws_external_resources.iam_role_arn,
                    account=net.aws_external_resources.account_id,
                )

    def _create_service_account(self, cluster_resource_name, hosts=None):
        # TODO: should be refactored to _create_cluster_service_account method

        # Support for porto will be added in https://st.yandex-team.ru/MDB-14040
        if not self._is_compute(hosts) and not self._is_aws(hosts):
            return

        if hosts is None:
            hosts = self.args['hosts']
        subcid = next(iter(hosts.values()))['subcid']

        service_account_name = 'cluster-agent-' + self.task['cid']
        self.iam.reconnect()
        service_account_id = self.iam.create_service_account(
            self.config.per_cluster_service_accounts.folder_id, service_account_name
        )

        self.iam.grant_role(
            service_account_id=service_account_id,
            role='internal.mdb.clusterAgent',
            cluster_resource_type=cluster_resource_name,
            cluster_id=self.task['cid'],
        )

        key_id, private_key = self.iam.create_key(service_account_id)
        self.pillar.exists(
            'subcid',
            subcid,
            ['data', 'service_account'],
            ['id', 'key_id', 'private_key'],
            [service_account_id, key_id, encrypt(self.config, private_key)],
        )

    def _create(self, *host_groups):
        """
        Generic method for host group create
        """
        self._desired_security_groups_state(host_groups)
        self._set_byoa_parameters(host_groups)
        self._set_network_parameters(host_groups)
        self._create_s3_bucket()
        for action in [
            self._create_cluster_service_account,
            self._create_encryption_stuff,
            self._create_conductor_group,
            self._create_host_secrets,
            self._create_host_group,
            self._issue_tls,
        ]:
            for host_group in host_groups:
                action(host_group)

        all_hosts = {}
        for host_group in host_groups:
            all_hosts.update(host_group.hosts)
        merged = build_host_group(SimpleNamespace(managed_zone=getattr(self.config.main, 'managed_zone')), all_hosts)
        self._wait_minions_registered(merged)
        self._before_highstate()
        self._highstate_host_group(merged)

        for action in [
            self._create_public_records,
            self._enable_monitoring,
        ]:
            for host_group in host_groups:
                action(host_group)
        monitoring_stub_request_id = self.args.get('monitoring_stub_request_id', None)

        if monitoring_stub_request_id is not None:
            self._sync_alerts_v2(self.task['cid'], monitoring_stub_request_id)
        else:
            self._sync_alerts(self.task['cid'])
