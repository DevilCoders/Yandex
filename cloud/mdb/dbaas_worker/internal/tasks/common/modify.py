"""
Common modify executor.
"""
from types import SimpleNamespace

from dbaas_common.worker import get_expected_resize_hours

from ...cloud_init import cloud_init_user_data
from ...providers.aws.ec2 import EC2, EC2DiscSpec, EC2InstanceChange
from ...providers.aws.route53 import Route53
from ...providers.common import Change
from ...providers.compute import ComputeApi, get_core_fraction
from ...providers.conductor import ConductorApi
from ...providers.dbm import DBMApi
from ...providers.deploy import DEPLOY_VERSION_V2, deploy_dataplane_host
from ...providers.disk_placement_group import DiskPlacementGroupProvider, DiskType
from ...providers.dns import DnsApi, Record
from ...providers.eds import EdsApi
from ...providers.juggler import JugglerApi
from ...providers.placement_group import PlacementGroupProvider
from ...providers.ssh import SSHClient
from ...providers.vpc import VPCProvider
from ...utils import get_conductor_root_group, get_eds_root, get_first_value, get_image_by_major_version
from ..utils import get_managed_hostname, get_private_hostname, resolve_order
from .deploy import BaseDeployExecutor


class BaseModifyExecutor(BaseDeployExecutor):
    """
    Generic class for modify executors
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.dbm_api = DBMApi(self.config, self.task, self.queue)
        self.compute_api = ComputeApi(self.config, self.task, self.queue)
        self.ec2 = EC2(self.config, self.task, self.queue)
        self.route53 = Route53(self.config, self.task, self.queue)
        self.conductor = ConductorApi(self.config, self.task, self.queue)
        self.eds_api = EdsApi(self.config, self.task, self.queue)
        self.disk_placement_groups = DiskPlacementGroupProvider(self.config, self.task, self.queue)
        self.placement_groups = PlacementGroupProvider(self.config, self.task, self.queue)
        self.dns_api = DnsApi(self.config, self.task, self.queue)
        self.ssh = SSHClient(self.config, self.task, self.queue)
        self.juggler = JugglerApi(self.config, self.task, self.queue)
        self.vpc = VPCProvider(self.config, self.task, self.queue)

    def _sync_conductor_group(self, host_group):
        """
        Modify conductor group for host group (if required)
        """
        group = getattr(host_group.properties, 'conductor_group_id', self.task['cid'])
        cond_root = get_conductor_root_group(host_group.properties, get_first_value(host_group.hosts))
        eds_root = get_eds_root(host_group.properties, get_first_value(host_group.hosts))
        self.eds_api.group_has_root(group, eds_root)
        self.conductor.group_has_root(group, cond_root)

    def _create_managed_record(self, host, opts):
        """
        Create managed dns records for managed host
        """
        managed_records = [
            Record(
                address=self.compute_api.get_instance_setup_address(host),
                record_type='AAAA',
            ),
        ]
        self.dns_api.set_records(get_managed_hostname(host, opts), managed_records)

    def _update_other_hosts_metadata(self, exclude_host):
        """
        Update metadata on all hosts with exclusion
        """
        deploys = []
        for host in self.args['hosts']:
            if host == exclude_host:
                continue
            deploys.append(
                self._run_operation_host(
                    host,
                    'metadata',
                    self.args['hosts'][host]['environment'],
                    pillar={'skip-billing': True},
                    title=f'metadata-skip-{exclude_host}',
                )
            )

        self.deploy_api.wait(deploys)

    def _compute_host_nbs_size_up(self, host, opts, changes):
        """
        Compute host upscale nbs size (with optional resources change)
        """
        pre_restart_timeout = opts.get('pre_restart_timeout', 10 * 60)
        post_restart_timeout = opts.get('post_restart_timeout', 10 * 60)
        pre_restart_run = opts.get('pre_restart_run', True)
        if pre_restart_run:
            self.deploy_api.wait(
                [
                    self._run_operation_host(
                        host, 'run-pre-restart-script', opts['environment'], timeout=pre_restart_timeout
                    )
                ]
            )
        self.compute_api.operation_wait(self.compute_api.instance_stopped(host, opts['vtype_id']))
        disks = self.compute_api.get_instance_disks(host, opts['vtype_id'])
        target_disk_id = disks['secondary'][0].id
        self.compute_api.operation_wait(self.compute_api.set_disk_size(target_disk_id, opts['space_limit']))
        if 'resources' in changes:
            self.compute_api.operation_wait(
                self.compute_api.set_instance_resources(
                    opts['vtype_id'],
                    opts['platform_id'],
                    opts['cpu_limit'],
                    get_core_fraction(opts['cpu_guarantee'], opts['cpu_limit']),
                    opts['io_cores_limit'],
                    opts['memory_limit'],
                )
            )
        self.compute_api.operation_wait(self.compute_api.instance_running(host, opts['vtype_id']))
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host, 'run-post-restart-disk-resize-script', opts['environment'], timeout=post_restart_timeout
                ),
            ]
        )

    def _compute_host_nbs_size_down(self, host, opts, changes):
        """
        Compute host downscale nbs size (with optional resources change)
        """
        timeout = int(get_expected_resize_hours(opts['space_limit'], 1, 1200).total_seconds())
        self.juggler.downtime_exists(get_managed_hostname(host, opts), timeout)
        placement_info = self.disk_placement_groups.get_placement_info(
            DiskType(opts['disk_type_id']),
            folder_id=self.config.compute.folder_id,
            geo=opts['geo'],
            cid=self.task['cid'],
            subcid=opts['subcid'],
            shard_id=opts['shard_id'],
        )
        create_disk_op, disk_id = self.compute_api.create_disk(
            opts['geo'],
            opts['space_limit'],
            opts['disk_type_id'],
            f'create-disk-for-{host}-resize',
            disk_placement_group_id=placement_info.disk_placement_group_id,
        )
        self.compute_api.operation_wait(create_disk_op)
        pre_restart_run = opts.get('pre_restart_run', True)
        if pre_restart_run:
            self.deploy_api.wait(
                [self._run_operation_host(host, 'run-pre-restart-script', opts['environment'], timeout=10 * 60)]
            )
        self.compute_api.operation_wait(self.compute_api.instance_stopped(host, opts['vtype_id']))
        self.compute_api.operation_wait(self.compute_api.attach_disk(opts['vtype_id'], disk_id))
        self.compute_api.operation_wait(self.compute_api.instance_running(host, opts['vtype_id']))
        self.deploy_api.wait(
            [self._run_operation_host(host, 'run-data-move-clone-script', opts['environment'], timeout=timeout)]
        )
        disks = self.compute_api.get_instance_disks(host, opts['vtype_id'])
        old_disk_id = next(disk.id for disk in disks['secondary'] if disk.id != disk_id)
        self.compute_api.operation_wait(
            self.compute_api.instance_stopped(host, opts['vtype_id'], context_suffix='stop2')
        )
        if 'resources' in changes:
            self.compute_api.operation_wait(
                self.compute_api.set_instance_resources(
                    opts['vtype_id'],
                    opts['platform_id'],
                    opts['cpu_limit'],
                    get_core_fraction(opts['cpu_guarantee'], opts['cpu_limit']),
                    opts['io_cores_limit'],
                    opts['memory_limit'],
                )
            )
        self.compute_api.operation_wait(self.compute_api.detach_disk(opts['vtype_id'], old_disk_id))
        delete_disk_op = self.compute_api.delete_disk(old_disk_id)
        self.compute_api.operation_wait(
            self.compute_api.instance_running(host, opts['vtype_id'], context_suffix='start2')
        )
        disk_placement_group_num = opts.get('pg_num', None)
        if disk_placement_group_num is not None:
            self.compute_api.save_disk_placement_group_meta(
                placement_info.disk_placement_group_id, disk_placement_group_num
            )
            self.compute_api.save_disk_meta(host, disk_placement_group_num, opts['disk_mount_point'])
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'run-post-restart-script',
                    opts['environment'],
                    pillar={'post-restart-timeout': timeout},
                    timeout=timeout,
                )
            ]
        )
        self.compute_api.operation_wait(delete_disk_op)

    def _compute_host_nbs_resources(self, host, opts):
        """
        Compute host resources change (nbs disks only)
        """
        pre_restart_timeout = opts.get('pre_restart_timeout', 10 * 60)
        post_restart_timeout = opts.get('post_restart_timeout', 10 * 60)
        pre_restart_run = opts.get('pre_restart_run', True)
        if pre_restart_run:
            self.deploy_api.wait(
                [
                    self._run_operation_host(
                        host, 'run-pre-restart-script', opts['environment'], timeout=pre_restart_timeout
                    )
                ]
            )
        self.compute_api.operation_wait(self.compute_api.instance_stopped(host, opts['vtype_id']))
        self.compute_api.operation_wait(
            self.compute_api.set_instance_resources(
                opts['vtype_id'],
                opts['platform_id'],
                int(opts['cpu_limit']),
                get_core_fraction(opts['cpu_guarantee'], opts['cpu_limit']),
                opts['io_cores_limit'],
                opts['memory_limit'],
            )
        )
        self.compute_api.operation_wait(self.compute_api.instance_running(host, opts['vtype_id']))
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host, 'run-post-restart-script', opts['environment'], timeout=post_restart_timeout
                )
            ]
        )

    def _compute_host_local_change(self, host, opts, properties):
        """
        Compute host resize on local disks
        """
        timeout = int(get_expected_resize_hours(opts['space_limit'], 1, 1200).total_seconds())
        self.juggler.downtime_exists(get_managed_hostname(host, opts), timeout)
        vtype_id = opts['vtype_id']
        context_data = self.compute_api.context_get(f'{host}-disks-save-before-delete')
        if context_data:
            boot_disk_id = context_data['boot_disk_id']
            disk_id = context_data['disk_id']
            if vtype_id == context_data['vtype_id']:
                # We interrupted just before delete
                self.compute_api.operation_wait(self.compute_api.instance_absent(host, vtype_id))
        elif not vtype_id:
            raise RuntimeError(f'Instance {host} was deleted. No disk info found in context.')
        else:
            disks = self.compute_api.get_instance_disks(host, vtype_id)
            if len(disks['secondary']) > 1:
                raise RuntimeError(
                    'Already attached {num_disks} secondary disks for host {host}, more than one cannot reuse'.format(
                        num_disks=len(disks['secondary']), host=host
                    )
                )
            # reuse already existed disk, state in root should help to continue operation
            disk_id = disks['secondary'][0].id if len(disks['secondary']) == 1 else None
            if not disk_id:
                # NBS space limit in MB (rounded by 4MB), but local-disk space limit at bound GiB
                # (it's actual for resize down)
                chunk_size = 4 * (1024**2)
                chunks = opts['space_limit'] // chunk_size + 1
                tmp_size = chunk_size * chunks
                create_disk_op, disk_id = self.compute_api.create_disk(
                    opts['geo'],
                    tmp_size,
                    'network-ssd',
                    f'create-disk-for-{host}-resize',
                )
                self.compute_api.operation_wait(create_disk_op)
                self.compute_api.operation_wait(self.compute_api.attach_disk(vtype_id, disk_id))
            self.deploy_api.wait(
                [self._run_operation_host(host, 'run-data-move-front-script', opts['environment'], timeout=timeout)]
            )
            # detach disk for new instance
            boot_disk_id = disks['boot'].id
            self.compute_api.operation_wait(self.compute_api.detach_disk(vtype_id, disk_id))
            self.compute_api.operation_wait(self.compute_api.instance_stopped(host, vtype_id))
            self.compute_api.operation_wait(
                self.compute_api.set_autodelete_for_boot_disk(vtype_id, boot_disk_id, False)
            )
            self.compute_api.add_change(
                Change(
                    f'{host}-disks-save-before-delete',
                    True,
                    context={
                        f'{host}-disks-save-before-delete': {
                            'boot_disk_id': boot_disk_id,
                            'disk_id': disk_id,
                            'vtype_id': vtype_id,
                        }
                    },
                    rollback=Change.noop_rollback,
                )
            )
            self.compute_api.operation_wait(self.compute_api.instance_absent(host, vtype_id))
        metadata = {
            'user-data': cloud_init_user_data(
                fqdn=host,
                deploy_version=str(self.config.deploy.version),
                mdb_deploy_api_host=deploy_dataplane_host(self.config),
            )
        }
        if getattr(properties, 'enable_oslogin', False):
            metadata['enable-oslogin'] = 'true'
        self.compute_api.operation_wait(
            self.compute_api.instance_exists(
                opts['geo'],
                host,
                get_managed_hostname(host, opts),
                get_image_by_major_version(
                    image_template=getattr(properties, 'compute_image_type_template', None),
                    image_fallback=properties.compute_image_type,
                    task_args=self.args,
                ),
                opts['platform_id'],
                opts['cpu_limit'],
                get_core_fraction(opts['cpu_guarantee'], opts['cpu_limit']),
                opts['memory_limit'],
                opts['subnet_id'],
                opts['assign_public_ip'],
                self.desired_security_groups,
                opts['disk_type_id'],
                opts['space_limit'],
                gpus=opts.get('gpu_limit', 0),
                io_cores=opts.get('io_cores_limit', 0),
                service_account_id=opts.get('service_account_id'),
                metadata=metadata,
                host_group_ids=opts['host_group_ids'],
                # we can not set autoDelete to True for disks, because it will deleted in case of error
                bootDiskSpec={
                    'autoDelete': False,
                    'diskId': boot_disk_id,
                },
                # we can not set autoDelete to True for disks, because it will deleted in case of error
                secondaryDiskSpecs=[
                    {
                        'autoDelete': False,
                        'diskId': disk_id,
                    }
                ],
            )
        )
        self._create_managed_record(host, opts)
        vtype_id = self.compute_api.save_instance_meta(host)
        if getattr(properties, 'use_superflow_v22', False):
            self.vpc.set_superflow_v22_flag_on_instance(vtype_id)
        # we need to start\stop instance because compute can not update bootdisk on running instance
        self.compute_api.operation_wait(self.compute_api.instance_stopped(host, vtype_id, context_suffix="stop2"))
        self.compute_api.operation_wait(self.compute_api.set_autodelete_for_boot_disk(vtype_id, boot_disk_id, True))
        self.compute_api.operation_wait(self.compute_api.instance_running(host, vtype_id))
        self.deploy_api.wait(
            [self._run_operation_host(host, 'run-data-move-back-script', opts['environment'], timeout=timeout)]
        )
        self.compute_api.operation_wait(self.compute_api.detach_disk(vtype_id, disk_id))
        self.compute_api.operation_wait(self.compute_api.delete_disk(disk_id))
        self._update_other_hosts_metadata(host)
        self.deploy_api.wait([self._run_operation_host(host, 'metadata-common', opts['environment'], timeout=10 * 60)])
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'run-post-restart-service-reload-script',
                    opts['environment'],
                    pillar={'post-restart-timeout': timeout},
                    timeout=timeout,
                ),
            ]
        )
        self.deploy_api.wait([self._run_operation_host(host, 'metadata', opts['environment'], timeout=10 * 60)])

    def _compute_host_security_groups_change(self, host):
        """
        Compute instance attribute change
        """
        self.compute_api.operation_wait(
            self.compute_api.ensure_instance_sg_same(
                fqdn=host,
                security_groups_ids=self.desired_security_groups,
            )
        )

    def _compute_host_attribute_change(self, host, opts):
        """
        Compute instance attribute change
        """
        self.compute_api.operation_wait(
            self.compute_api.update_instance_attributes(
                fqdn=host,
                service_account_id=opts['service_account_id'],
            )
        )

    def _compute_host_state_change(self, host, opts):
        """
        Compute instance state change
        """
        operation = self.compute_api.instance_running(host, opts['vtype_id'], context_suffix='modify-state-start')
        if operation:
            self.compute_api.operation_wait(operation)
            post_restart_timeout = opts.get('post_restart_timeout', 10 * 60)
            self.deploy_api.wait(
                [
                    self._run_operation_host(
                        host, 'run-post-restart-disk-resize-script', opts['environment'], timeout=post_restart_timeout
                    ),
                ]
            )

    def _compute_host_recreate(self, host, opts, host_group):
        """
        Recreate host in compute for resize
        """
        pre_restart_timeout = opts.get('pre_restart_timeout', 10 * 60)
        pre_restart_run = opts.get('pre_restart_run', True)
        self.juggler.downtime_exists(get_managed_hostname(host, opts), self.deploy_api.get_seconds_to_deadline())
        context_data = self.compute_api.context_get(f'{host}-recreated')
        if not context_data:
            if pre_restart_run:
                self.deploy_api.wait(
                    [
                        self._run_operation_host(
                            host, 'run-pre-restart-script', opts['environment'], timeout=pre_restart_timeout
                        )
                    ]
                )
            self.compute_api.operation_wait(self.compute_api.instance_absent(host))
            deploy_version = self.deploy_api.get_deploy_version_from_minion(host)
            if deploy_version == DEPLOY_VERSION_V2:
                self.deploy_api.unregister_minion(host)
            else:
                raise RuntimeError(f'Unsupported deploy version for {host}: {deploy_version}')
        self.compute_api.add_change(
            Change(
                f'{host}-recreated',
                True,
                context={f'{host}-recreated': True},
                rollback=Change.noop_rollback,
            )
        )
        metadata = {
            'user-data': cloud_init_user_data(
                fqdn=host,
                deploy_version=str(self.config.deploy.version),
                mdb_deploy_api_host=deploy_dataplane_host(self.config),
            )
        }
        if getattr(host_group.properties, 'enable_oslogin', False):
            metadata['enable-oslogin'] = 'true'
        placement_info = self.disk_placement_groups.get_placement_info(
            DiskType(opts['disk_type_id']),
            folder_id=self.config.compute.folder_id,
            geo=opts['geo'],
            cid=self.task['cid'],
            subcid=opts['subcid'],
            shard_id=opts['shard_id'],
        )
        self.compute_api.operation_wait(
            self.compute_api.instance_exists(
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
                disk_placement_group_id=placement_info.disk_placement_group_id,
                host_group_ids=opts['host_group_ids'],
                vhost_net=opts.get('vhost_net', False),
                placement_group_id=opts.get('placement_group_id', None),
            ),
            timeout=50 * 60,
        )
        self._create_managed_record(host, opts)
        vtype_id = self.compute_api.save_instance_meta(host)
        if getattr(host_group.properties, 'use_superflow_v22', False):
            self.vpc.set_superflow_v22_flag_on_instance(vtype_id)
        disk_placement_group_num = opts.get('pg_num', None)
        if disk_placement_group_num is not None:
            self.compute_api.save_disk_placement_group_meta(
                placement_info.disk_placement_group_id, disk_placement_group_num
            )
            self.compute_api.save_disk_meta(host, disk_placement_group_num, opts['disk_mount_point'])
        self._update_other_hosts_metadata(host)
        self.deploy_api.wait_minions_registered(host)
        pillar = host_group.properties.create_host_pillar.copy()
        pillar.update(
            {'sync-timeout': self.deploy_api.get_seconds_to_deadline() // self.deploy_api.default_max_attempts}
        )
        self.deploy_api.wait([self.deploy_api.run(host, pillar=pillar, deploy_title='recreate')])

    def _change_compute_host(self, host, opts, host_group, changes):
        """
        Perform single host change in compute
        """
        recreate = False
        start = False
        if 'state' in changes:
            changes.remove('state')
            start = True
        if 'nbs_size_up' in changes:
            self._compute_host_nbs_size_up(host, opts, changes)
        elif 'nbs_size_down' in changes:
            if self._is_compute_recreate_safe(host_group, opts.get('host_recreate_force', False)):
                recreate = True
                self._compute_host_recreate(host, opts, host_group)
            else:
                self._compute_host_nbs_size_down(host, opts, changes)
        elif changes - {'instance_attribute', 'instance_network'} == {'resources'} and not opts[
            'disk_type_id'
        ].startswith('local-'):
            self._compute_host_nbs_resources(host, opts)
        elif changes - {'instance_attribute', 'instance_network'} == {'resources'} and not opts[
            'disk_type_id'
        ].startswith('network-'):
            recreate = True
            if self._is_compute_recreate_safe(host_group, opts.get('host_recreate_force', False)):
                self._compute_host_recreate(host, opts, host_group)
            else:
                self._compute_host_local_change(host, opts, host_group.properties)
        elif 'nvme_size' in changes:
            recreate = True
            if self._is_compute_recreate_safe(host_group, opts.get('host_recreate_force', False)):
                self._compute_host_recreate(host, opts, host_group)
            else:
                self._compute_host_local_change(host, opts, host_group.properties)
        elif changes not in [{'instance_attribute'}, {'instance_attribute', 'instance_network'}, {'instance_network'}]:
            raise RuntimeError('Unknown changes: {changes} for host {host}'.format(changes=changes, host=host))

        if start:
            self._compute_host_state_change(host, opts)
        if 'instance_attribute' in changes and not recreate:
            self._compute_host_attribute_change(host, opts)
        if 'instance_network' in changes and not recreate:
            self._compute_host_security_groups_change(host)

    def _change_aws_host(
        self, host: str, opts: dict, host_group: SimpleNamespace, changes: set[EC2InstanceChange]
    ) -> None:
        def deploy_and_wait(operation: str) -> None:
            self.deploy_api.wait(
                [
                    self._run_operation_host(host, operation, opts['environment'], timeout=10 * 60),
                ]
            )

        region_name = opts['region_name']
        instance_with_region = dict(
            instance_id=opts['vtype_id'],
            region_name=region_name,
        )

        if EC2InstanceChange.disk_size_up in changes:
            self.ec2.set_instance_data_disk_size(
                data_disk_size=opts['space_limit'],
                **instance_with_region,
            )
            deploy_and_wait('data-disk-resize')
        if EC2InstanceChange.instance_type in changes:
            # as in Yandex.Cloud, instance_type change require stopped instance
            deploy_and_wait('run-pre-restart-script')
            self.ec2.instance_stopped(**instance_with_region)
            self.ec2.instance_wait_until_stopped(**instance_with_region)
            self.ec2.update_instance_type(
                instance_type=opts['cloud_provider_flavor_name'],
                **instance_with_region,
            )
            self.ec2.instance_running(**instance_with_region)
            self.ec2.instance_wait_until_running(**instance_with_region)
            # public and private IPs may change after instance restart
            addresses = self.ec2.get_instance_addresses(**instance_with_region)
            self.route53.set_records_in_public_zone(host, addresses.public_records())
            self.route53.set_records_in_public_zone(get_private_hostname(host, opts), addresses.private_records())
            deploy_and_wait('run-post-restart-script')

    def _dbm_transfer(self, transfer, space_limit, env):
        """
        Handle dbm transfer
        """
        action = 'initiated'
        if transfer.get('continue'):
            action = 'continued'
        context_id = f"transfer.{transfer['src_dom0']}.{transfer['container']}"
        if not self.ssh.context_get(context_id):
            self.ssh.add_change(Change(context_id, action, context={context_id: True}, rollback=Change.noop_rollback))
            with self.ssh.get_conn(transfer['src_dom0'], use_agent=True) as conn:
                self.ssh.exec_command(
                    transfer['src_dom0'],
                    conn,
                    '/usr/sbin/move_container.py --pre-stop-state {state} {fqdn}'.format(
                        fqdn=transfer['container'], state='components.dbaas-operations.run-pre-restart-script'
                    ),
                    interruptable=True,
                )
        else:
            self.ssh.add_change(Change(context_id, action, rollback=Change.noop_rollback))
        self._update_other_hosts_metadata(transfer['container'])
        timeout = int(get_expected_resize_hours(space_limit, 1, 1200).total_seconds())
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    transfer['container'],
                    'run-post-restart-script',
                    env,
                    pillar={'post-restart-timeout': timeout},
                    timeout=timeout,
                )
            ]
        )

    def _is_compute_recreate_safe(self, host_group, force_true=False):
        """
        Check if recreate for resize is safe
        """
        if force_true:
            return True
        if len(host_group.hosts) == 1:
            self.logger.info('single host in host group. recreate for resize is not safe')
            return False
        if not host_group.properties.compute_recreate_on_resize:
            self.logger.info('Recreate for resize is not safe for this host type')
            if 'MDB_FORCE_UNSAFE_RESIZE' in self.task['feature_flags']:
                self.logger.info('Using unsafe resize anyway (forced by feature flag)')
                return True
            return False
        return True

    def _change_host_group(
        self,
        host_group,
        order=None,
        health_check=None,
        allow_fail_hosts=0,
        do_before_recreate=None,
        do_after_recreate=None,
    ):
        """
        Change hosts in dbm or compute
        """
        metadata_pending = []

        self._sync_conductor_group(host_group)
        self._desired_security_groups_state([host_group])
        for host in resolve_order(order, 'change') if order else host_group.hosts:
            opts = host_group.hosts[host]
            if opts['vtype'] == 'porto':
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
                    },
                    volumes={
                        '/': {
                            'space_limit': host_group.properties.rootfs_space_limit,
                        },
                        host_group.properties.dbm_data_path: {
                            'space_limit': opts['space_limit'],
                        },
                    },
                    platform_id=opts['platform_id'],
                    bootstrap_cmd=get_image_by_major_version(
                        image_template=getattr(host_group.properties, 'dbm_bootstrap_cmd_template', None),
                        image_fallback=host_group.properties.dbm_bootstrap_cmd,
                        task_args=self.args,
                    ),
                )
                if change:
                    if change.transfer:
                        if do_before_recreate:
                            do_before_recreate()
                        self._dbm_transfer(change.transfer, opts['space_limit'], opts['environment'])
                        self.deploy_api.wait([self._run_operation_host(host, 'metadata', opts['environment'])])
                        if do_after_recreate:
                            do_after_recreate()
                        if health_check:
                            self._health_host_group(
                                host_group,
                                context_suffix=f'-after-{host}-upgrade',
                                allow_fail_hosts=allow_fail_hosts,
                            )
                    elif change.jid:
                        self.deploy_api.wait([change.jid])
                        metadata_pending.append(host)
                    else:
                        self.dbm_api.wait_operation(change.operation_id)
                        metadata_pending.append(host)
            elif opts['vtype'] == 'compute':
                if not opts['vtype_id'] and self.compute_api.get_instance(host):
                    opts['vtype_id'] = self.compute_api.save_instance_meta(host)
                changes = self.compute_api.get_instance_changes(
                    host,
                    opts['vtype_id'],
                    opts['platform_id'],
                    opts['cpu_limit'],
                    get_core_fraction(opts['cpu_guarantee'], opts['cpu_limit']),
                    opts['gpu_limit'],
                    opts['io_cores_limit'],
                    opts['memory_limit'],
                    opts['disk_type_id'],
                    opts['space_limit'],
                    opts.get('service_account_id'),
                    self.desired_security_groups,
                )
                if changes:
                    if do_before_recreate:
                        do_before_recreate()
                    self._change_compute_host(host, opts, host_group, changes)
                    self.deploy_api.wait([self._run_operation_host(host, 'metadata', opts['environment'])])
                    public_records = []
                    for address, version in self.compute_api.get_instance_public_addresses(host):
                        public_records.append(Record(address=address, record_type=('AAAA' if version == 6 else 'A')))
                    self.dns_api.set_records(host, public_records)
                    if do_after_recreate:
                        do_after_recreate()
                    if health_check:
                        self._health_host_group(host_group, context_suffix=f'-after-{host}-upgrade')
            elif opts['vtype'] == 'aws':
                changes = self.ec2.get_instance_changes(
                    instance_id=opts['vtype_id'],
                    data_disk=EC2DiscSpec(
                        type=opts['cloud_provider_disk_type'],
                        size=opts['space_limit'],
                    ),
                    instance_type=opts['cloud_provider_flavor_name'],
                    region_name=opts['region_name'],
                )
                if changes:
                    self._change_aws_host(host, opts, host_group, changes)
                    self.deploy_api.wait([self._run_operation_host(host, 'metadata', opts['environment'])])
            else:
                raise RuntimeError('Unknown vtype: {vtype} for host {host}'.format(vtype=opts['vtype'], host=host))

        if self.remove_allow_all_rule:  # we remove it after we added user groups to all hosts
            self._update_service_sg(allow_all=False)

        self.deploy_api.wait(
            [
                self._run_operation_host(host, 'metadata', self.args['hosts'][host]['environment'])
                for host in metadata_pending
            ]
        )
        if health_check:
            self._health_host_group(host_group)
        # TODO: drop
        self._sync_alerts(self.task['cid'])

    def _change_host_public_ip(self, host):
        if self.args['hosts'][host]['vtype'] != 'compute':
            return

        if self.compute_api.is_public_ip_changed(fqdn=host, use_public_ip=self.args['hosts'][host]['assign_public_ip']):
            self.compute_api.operation_wait(
                self.compute_api.set_instance_public_ip(
                    fqdn=host, use_public_ip=self.args['hosts'][host]['assign_public_ip']
                )
            )
            self._update_public_records(host)

    def _update_public_records(self, host):
        """
        Update public dns records for managed host
        """
        if self.args['hosts'][host]['vtype'] == 'compute':
            public_records = []
            for address, version in self.compute_api.get_instance_public_addresses(host):
                public_records.append(Record(address=address, record_type=('AAAA' if version == 6 else 'A')))
            self.dns_api.set_records(host, public_records)
