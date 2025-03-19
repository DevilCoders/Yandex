"""
Helpers for creating fake dataproc cluster without dataproc control-plane
"""

import os
import sh
import yaml
import signal
import logging
import tempfile
import ipaddress

from typing import List, Tuple, Optional
from . import (consts, compute, utils)

LOG = logging.getLogger('dataproc')
BOOTPATH = '/var/lib/cloud/scripts/per-boot/50-dataproc-start.sh'
PER_INSTANCE_INIT_ACTIONS = '/var/lib/cloud/scripts/per-instance/50-dataproc-init-actions.py'


class DataprocException(Exception):
    """
    Common exception for dataproc helper
    """


class DataprocServiceBrokenException(Exception):
    """
    Data Proc service broken
    """


def _get_pillar(cluster_id: str,
                masternodes: List[str],
                datanodes: List[str],
                computenodes: List[str],
                ssh_public_keys: List[str],
                resources: dict,
                services: List[str],
                properties: dict,
                initialization_actions: List[dict],
                region: str,
                log_group_id: str,
                logging_endpoint_url: str,
                custom_userdata: dict,
                bucket: str,
                ) -> str:
    subclusters = {
        name: data for name, data in {
            'subcid-master': {
                'name': 'masters',
                'role': consts.ROLE_MASTER,
                'hosts': [f'{i}.{region}.internal' for i in masternodes],
                'subcid': 'subcid-master',
                'resources': {
                    'core_fraction': resources['core_fraction'],
                    'cores': resources['cores'],
                    'memory': resources['memory'],
                },
            },
            'subcid-data': {
                'name': 'data',
                'role': consts.ROLE_DATA,
                'hosts': [f'{i}.{region}.internal' for i in datanodes],
                'subcid': 'subcid-data',
                'resources': {
                    'core_fraction': resources['core_fraction'],
                    'cores': resources['cores'],
                    'memory': resources['memory'],
                },
            },
            'subcid-compute': {
                'name': 'compute',
                'role': consts.ROLE_COMPUTE,
                'hosts': [f'{i}.{region}.internal' for i in computenodes],
                'subcid': 'subcid-compute',
                'resources': {
                    'core_fraction': resources['core_fraction'],
                    'cores': resources['cores'],
                    'memory': resources['memory'],
                },
            },
        }.items()
        if data['hosts']
    }

    userdata = {
        'ssh_authorized_keys': ssh_public_keys,
        'data': {
            # option needs to be false, cz otherwize tez-ui will go through ui_proxy interface
            'ui_proxy': False,
            'dataproc': {
                'version': 'testing',
            },
            'agent': {
                'ui_proxy_url': 'https://dataproc-ui.yandexcloud.net/',
                'cid': cluster_id,
            },
            's3_bucket': bucket,
            'logging': {'group_id': log_group_id, 'url': logging_endpoint_url},
            'services': services,
            'properties': properties,
            'initialization_actions': initialization_actions,
            'subcluster_main_id': 'subcid-master',
            'topology': {'subclusters': subclusters},
        },
    }
    _update_userdata(userdata, custom_userdata)
    return '#cloud-config\n' + yaml.dump(userdata)


def _update_userdata(userdata: dict, custom: dict):
    if not custom:
        return
    node = userdata
    for name, value in custom.items():
        node_keys = name.split(':')
        for node_key in node_keys[:-1]:
            node = node.setdefault(node_key, {})
        if node_keys[-1] in node:
            LOG.info('Updating userdata property %r with custom value %r', node[node_keys[-1]], value)
        node[node_keys[-1]] = value


def get_rsync_addr(ctx, name):
    """
    Return address for rsync
    ipv4 or [ipv6]
    """
    address = compute.get_public_ip_address(ctx, name)
    ip_address = ipaddress.ip_address(address)
    if isinstance(ip_address, ipaddress.IPv6Address):
        return f'[{address}]'
    return address


def sync_bootstrap(ctx, names):
    """
    Sync salt states, modules and dataproc bootstrap
    """
    tasks = []
    user = ctx.conf["user"]
    for name in names:
        addr = get_rsync_addr(ctx, name)
        tasks.append(
            (f'{name}:{BOOTPATH}', sh.rsync(*utils.rsync_options(ctx),
                                            '../bootstrap/dataproc-start.sh',
                                            f'{user}@{addr}:{BOOTPATH}',
                                            _bg=True,
                                            _timeout=120)))
        tasks.append(
            (f'{name}:{PER_INSTANCE_INIT_ACTIONS}', sh.rsync(*utils.rsync_options(ctx),
                                                             '../bootstrap/dataproc_init_actions.py',
                                                             f'{user}@{addr}:{PER_INSTANCE_INIT_ACTIONS}',
                                                             _bg=True,
                                                             _timeout=120)))
        tasks.append(
            (f'{name}:/srv/salt', sh.rsync(*utils.rsync_options(ctx),
                                           '../bootstrap/salt',
                                           f'{user}@{addr}:/srv/',
                                           _bg=True,
                                           _timeout=120)))
        tasks.append(
            (f'{name}:/srv/pillar', sh.rsync(*utils.rsync_options(ctx),
                                             '../bootstrap/pillar/common.sls', '../bootstrap/pillar/versions.sls',
                                             f'{user}@{addr}:/srv/pillar/',
                                             _bg=True,
                                             _timeout=120)))
        tasks.append(
            (f'{name}:dataproc-diagnostics.sh', sh.rsync(*utils.rsync_options(ctx),
                                                         '../bootstrap/dataproc-diagnostics.sh',
                                                         f'{user}@{addr}:/usr/local/bin/dataproc-diagnostics.sh',
                                                         _bg=True,
                                                         _timeout=120)))

    for name, p in tasks:
        try:
            p.wait()
            LOG.debug(f'{name} synced')
        except sh.ErrorReturnCode as exc:
            raise DataprocException(f'Failed to sync {name}', exc)


def download_logs(ctx, instance, timeout=120):
    """
    Fallback mechanism for collecting logs
    Returns a list of [name, task]
    """
    tasks = []
    user = ctx.conf['user']
    addr = get_rsync_addr(ctx, instance)
    for name, path in [
        ('dataproc-start.log', '/var/log/yandex/dataproc-start.log'),
        ('dataproc-init-actions.log', '/var/log/yandex/dataproc-init-actions.log'),
        ('cloud-init.log', '/var/log/cloud-init.log'),
        ('salt-minion', '/var/log/salt/minion'),
        ('syslog', '/var/log/syslog'),
    ]:
        tasks.append(
            (
                f'downloading {instance}:{name}',
                sh.rsync(
                    *utils.rsync_options(ctx),
                    f'{user}@{addr}:{path}',
                    f'artifacts/{instance}-{name}',
                    _bg=True,
                    _timeout=60),
            ),
        )
    return tasks


def bootstrap_boot(ctx, names, timeout=900):
    """
    Manually run dataproc-start.sh script
    """
    def _bootstrap(user, addr, timeout):
        """
        Run dataproc-start.sh script on addr
        """
        return sh.ssh(*utils.ssh_options(ctx),
                      f'{user}@{addr}',
                      f'sudo sh -c "FORCE=true {BOOTPATH} 2>&1 >> /var/log/yandex/dataproc-start.log"',
                      _bg=True,
                      _no_out=True,
                      _no_err=True,
                      _timeout=timeout)

    tasks = []
    for name in names:
        addr = compute.get_public_ip_address(ctx, name)
        LOG.debug(f'Executing {name}#{addr} dataproc-start.sh')
        tasks.append((name, _bootstrap(ctx.conf["user"], addr, timeout)))

    trace_tasks = []
    exceptions = []

    for name, run in tasks:
        try:
            run.wait()
        except (sh.TimeoutException, sh.ErrorReturnCode) as e:  # noqa
            # Wait bootstrap from all instances and collect logs only from failed ones.
            exceptions.append(e)
            trace_tasks.extend(download_logs(ctx, name))

    # Wait until all logs downloaded
    for name, run in trace_tasks:
        try:
            run.wait()
        except (sh.TimeoutException, sh.ErrorReturnCode) as e:
            LOG.warn(f'failed {name}', e)
    if exceptions:
        raise DataprocException('Failed bootstrap:', exceptions)


def bootstrap_once(ctx, names, timeout=900):
    """
    Manually run dataproc_init_actions.py script
    """
    def _bootstrap(user, addr, timeout):
        """
        Run dataproc_init_actions.py script on addr
        """
        return sh.ssh(*utils.ssh_options(ctx),
                      f'{user}@{addr}',
                      f'sudo sh -c '
                      f'"FORCE=true {PER_INSTANCE_INIT_ACTIONS} 2>&1 >> /var/log/yandex/dataproc-init-actions.log"',
                      _bg=True,
                      _no_out=True,
                      _no_err=True,
                      _timeout=timeout)

    tasks = []
    for name in names:
        addr = compute.get_public_ip_address(ctx, name)
        LOG.debug(f'Executing {name}#{addr} dataproc_init_actions.py')
        tasks.append((name, _bootstrap(ctx.conf["user"], addr, timeout)))

    trace_tasks = []
    exceptions = []

    for name, run in tasks:
        try:
            run.wait()
        except (sh.TimeoutException, sh.ErrorReturnCode) as e:  # noqa
            # Wait bootstrap from all instances and collect logs only from failed ones.
            exceptions.append(e)
            trace_tasks.extend(download_logs(ctx, name))

    # Wait until all logs downloaded
    for name, run in trace_tasks:
        try:
            run.wait()
        except (sh.TimeoutException, sh.ErrorReturnCode) as e:
            LOG.warn(f'failed {name}', e)
    if exceptions:
        raise DataprocException('Failed bootstrap:', exceptions)


def setup_firewall(ctx, names):
    """
    Manually enable firewall and block all incoming packets except tcp/22 and testing network
    """
    v4_cidr_block = ctx.conf['vpc']['v4_cidr_block']
    isolated_network = ctx.state.get('isolated_network', True)
    user = ctx.conf['user']
    for name in names:
        addr = compute.get_public_ip_address(ctx, name)
        try:
            sh.ssh(*utils.ssh_options(ctx),
                   f'{user}@{addr}',
                   f'sudo ufw allow ssh && '
                   f'sudo ufw allow from {v4_cidr_block} && '
                   f'sudo ufw allow 546:547/udp && '  # dhcpv6
                   f'sudo ufw allow 123/udp && '  # ntp
                   f'sudo ufw --force enable')
            if isolated_network:
                sh.ssh(*utils.ssh_options(ctx),
                       f'{user}@{addr}',
                       f'sudo ufw default deny outgoing && '
                       f'sudo ufw allow out 53 && '  # dns
                       f'sudo ufw allow out to {v4_cidr_block} && '
                       f'sudo ufw allow out to 169.254.169.254 && '  # metadata service
                       f'sudo ufw allow out 546:547/udp && '  # dhcpv6
                       f'sudo ufw allow out 123/udp && '  # ntp
                       f'sudo ufw reload')

        except (sh.TimeoutException, sh.ErrorReturnCode) as exc:
            raise DataprocException('Failed to setup firewall', exc)


def all_instances_are_created(ctx, names: List[str]):
    for name in names:
        try:
            compute.get_instance_by_name(ctx, name)
        except compute.InstanceNotFound:
            return False
    return True


def create_dataproc_instances(ctx,
                              userdata: str,
                              names: List[str],
                              timeout: int,
                              init_timeout: int = 0):
    """
    Asynchronous create many instances for dataproc cluster
    """
    operations = []

    if all_instances_are_created(ctx, names) and ctx.state.get('cluster_precreated'):
        return
    # Create instances
    for name in names:
        operation = compute.instance_create(ctx,
                                            name=name,
                                            metadata={
                                                'serial-port-enable': '1',
                                                'user-data': userdata,
                                                'enable-oslogin': 'true',
                                            },
                                            ipv6=ctx.state['use_ipv6'])
        operations.append(operation)
    for operation in operations:
        compute.instance_wait_create(ctx, operation)
    # Wait until tcp/22 for ssh will be available
    for name in names:
        compute.wait_ssh_available(ctx, ctx.conf['user'], compute.get_public_ip_address(ctx, name))
    setup_firewall(ctx, names)
    sync_bootstrap(ctx, names)
    bootstrap_boot(ctx, names, timeout)
    if init_timeout:
        bootstrap_once(ctx, names, init_timeout)


def save_diagnostics(ctx):
    """
    Save diagnostics for instances with names
    """
    user = ctx.conf['user']
    feature_name = ctx.feature.name.replace(' ', '_')
    scenario_name = ctx.scenario.name.replace(' ', '_')

    for _, cluster in ctx.state['clusters'].items():
        instances = set(cluster['masternodes'] + cluster['datanodes'] + cluster['computenodes'])
        tasks = []
        for instance_name in instances:
            try:
                addr = compute.get_public_ip_address(ctx, instance_name)
                tasks.append(
                    (instance_name, sh.ssh(*utils.ssh_options(ctx),
                                           f'{user}@{addr}',
                                           'sudo DPKG_VERIFY=false /usr/local/bin/dataproc-diagnostics.sh',
                                           _bg=True,
                                           _timeout=300)))
            except compute.InstanceNotFound:
                LOG.warn(f'Can\'t save diagnostics for instance {instance_name}, instance not found')
        for instance_name, command in tasks:
            LOG.debug(f'Waiting diagnostics for instance {instance_name}')
            path = f'artifacts/{feature_name}/{scenario_name}'
            try:
                # Create directory for feature/scenario if not exists
                if not os.path.exists(path):
                    os.makedirs(path)
                command.wait()
                addr = get_rsync_addr(ctx, instance_name)
                sh.rsync(
                    *utils.rsync_options(ctx),
                    f'{user}@{addr}:/home/{user}/diagnostics.tgz',
                    f'{path}/{instance_name}.tar.gz',
                    _timeout=60)
                # Create directory for intance if not exists
                instance_path = f'{path}/{instance_name}'
                if not os.path.exists(instance_path):
                    os.makedirs(instance_path)
                sh.tar('-xf', f'{path}/{instance_name}.tar.gz', '--directory', instance_path)
                sh.rm(f'{path}/{instance_name}.tar.gz')
            except (sh.TimeoutException, sh.ErrorReturnCode):
                LOG.warning(f'Failed to collect diagnostics for instance {instance_name}', exc_info=True)


def service_is_running(ctx, instances, service):
    """
    Method verifies, that services has expected status
    """
    tasks = []
    for instance in instances:
        addr = compute.get_public_ip_address(ctx, instance)
        tasks.append((instance, sh.ssh(*utils.ssh_options(ctx),
                                       f'{ctx.conf["user"]}@{addr}',
                                       f'service {service} status',
                                       _bg=True,
                                       _timeout=60)))
    ret = {}
    for instance, command in tasks:
        try:
            command.wait()
            ret[instance] = True
        except (sh.TimeoutException, sh.ErrorReturnCode):
            code, out, err = str(command.exit_code), str(command.stdout), str(command.stderr)
            LOG.warning(f'service {service} on instance {instance} is broken, code: {code}, out: {out}, err: {err}')
            ret[instance] = False
    return ret


def package_installed(ctx, instances, package):
    """
    Method verifies, that package installed on instances
    """
    tasks = []
    for instance in instances:
        addr = compute.get_public_ip_address(ctx, instance)
        tasks.append((instance, sh.ssh(*utils.ssh_options(ctx),
                                       f'{ctx.conf["user"]}@{addr}',
                                       f'apt -qq list {package}',
                                       _bg=True,
                                       _timeout=60)))
    ret = {}
    for instance, command in tasks:
        # Let's assume that package not installed
        # Example of not installed package:
        # ubuntu@dp-itxhgweb-ppa-m0:~$ apt -qq list python3.5 ; echo $?
        # python3.5/focal 3.5.10-1+focal2 amd64
        # 0
        ret[instance] = False
        try:
            command.wait()
            if '[installed]' in str(command.stdout) and int(command.exit_code) == 0:
                # Example of installed package:
                # ubuntu@dp-itxhgweb-ppa-m0:~$ apt -qq list python3.6 ; echo $?
                # python3.6/focal,now 3.6.14-1+focal1 amd64 [installed]
                # 0
                ret[instance] = True
        except (sh.TimeoutException):
            code, out, err = str(command.exit_code), str(command.stdout), str(command.stderr)
            raise DataprocException(f'Failed package_installed on {instance}, code: {code}, out: {out}, err: {err}',
                                    exc_info=True)
    return ret


def execute_command(ctx, instance, command):
    """
    Asynchronous execute command on instance
    """
    addr = compute.get_public_ip_address(ctx, instance)
    LOG.debug(f'Starting command `{command}` on `{instance}`')

    ctx.state['command'] = {
        'process': sh.ssh(*utils.ssh_options(ctx),
                          f'{ctx.conf["user"]}@{addr}',
                          command,
                          _bg=True,
                          _timeout_signal=signal.SIGTERM),
        'exit_code': None,
        'timeout': False,
        'stdout': None,
        'stderr': None,
    }


def wait_command(ctx, timeout=60.0):
    """
    Wait until command finish on instance
    """
    process = ctx.state['command']['process']
    try:
        process.wait(timeout)
    except sh.ErrorReturnCode:
        LOG.debug(f'Command finished with exit_code {process.exit_code}')
    except sh.TimeoutException:
        LOG.debug('Command timed out')
        ctx.state['command']['timeout'] = True
    ctx.state['command']['exit_code'] = process.exit_code
    ctx.state['command']['stdout'] = process.stdout
    ctx.state['command']['stderr'] = process.stderr
    # Sometimes python-sh fails by timeout, but actually sh executed with 0 exit code
    # So, i've added wonderful solution for this case.
    if process.exit_code == 0 and ctx.state['command']['timeout']:
        LOG.warn(f'Command failed with timeout, but actually executed, {ctx.state["command"]}')
        ctx.state['command']['timeout'] = False


def generate_instance_names(prefix: str,
                            topology: Tuple[int]) -> Tuple[List[str]]:
    """
    Generate tuple of instance names
    ([m1], [d1,d2], [c1,c2,c3])
    for cluster with 1 masternode, 2 data nodes, and 3 compute nodes
    """
    m, d, c = topology
    return (
        [f'{prefix}-m{x}' for x in range(m)],
        [f'{prefix}-d{x}' for x in range(d)],
        [f'{prefix}-c{x}' for x in range(c)],
    )


def fill_roles(ctx,
               cluster_id: str,
               topology: Tuple[int]):
    masternodes, datanodes, computenodes = generate_instance_names(cluster_id, topology)
    ctx.state['clusters'][cluster_id] = {
        'masternodes': masternodes,
        'datanodes': datanodes,
        'computenodes': computenodes,
    }
    return masternodes, datanodes, computenodes


def dataproc_create(ctx,
                    name: str,
                    services: List[str],
                    properties: dict,
                    initialization_actions: List[dict],
                    timeout: int,
                    topology: Optional[Tuple[int]]):
    """
    Create fake dataproc cluster within compute instances
    """
    if not topology:
        # Default topology with 1 master, 1 data and 1 compute node
        topology = (1, 1, 1)
    cluster_id = f'{ctx.id}-{name}'
    masternodes, datanodes, computenodes = fill_roles(ctx=ctx, cluster_id=cluster_id, topology=topology)
    init_timeout = 0
    for init_act in initialization_actions:
        init_timeout += init_act.get("timeout", 600)
    userdata = _get_pillar(cluster_id,
                           masternodes,
                           datanodes,
                           computenodes,
                           [ctx.state['ssh_public_key']],
                           ctx.conf['compute'],
                           services=services,
                           properties=properties,
                           initialization_actions=initialization_actions,
                           region=ctx.conf['environment']['region'],
                           log_group_id=ctx.conf['environment']['log_group_id'],
                           logging_endpoint_url=ctx.conf['environment']['logging_endpoint_url'],
                           custom_userdata=ctx.state.get('userdata'),
                           bucket=ctx.id,
                           )
    create_dataproc_instances(ctx, userdata, masternodes + datanodes + computenodes, timeout, init_timeout)


def dataproc_restart(ctx,
                     name: str):
    """
    Restart all instances of cluster
    """
    instances = []
    for _, subcluster in ctx.state['clusters'][f'{ctx.id}-{name}'].items():
        instances.extend(subcluster)

    # Restart all instances
    compute.instances_stop(ctx, instances)
    compute.instances_start(ctx, instances)
    for name in instances:
        compute.wait_ssh_available(ctx, ctx.conf['user'], compute.get_public_ip_address(ctx, name))
    bootstrap_boot(ctx, instances, timeout=300)


def default_properties(ctx):
    """
    """
    return {
        'core': {
            'fs.s3a.endpoint': ctx.conf['s3']['endpoint'],
        },
        'dataproc': {
            'disable_highstate': True,
            'disable_agent': False,
        },
        'yarn': {
            'yarn.nm.liveness-monitor.expiry-interval-ms': 15000,
            'yarn.log-aggregation-enable': 'false',
        },
    }


def dataproc_clean(ctx):
    """
    Clean dataproc instances
    """
    to_delete = []
    for name, cluster in ctx.state['clusters'].items():
        LOG.debug(f'Planning to delete dataproc cluster {name}')
        for _, instances in cluster.items():
            to_delete.extend(instances)
    compute.instances_delete(ctx, to_delete)
    ctx.state['clusters'] = {}


def create_file(ctx, instances: List[str], file_path: str, file_content: str, permissions: Optional[str] = None):
    """
    Create file on list of instances
    """
    user = ctx.conf["user"]
    if permissions is None:
        permissions = '0744'
    with tempfile.NamedTemporaryFile(mode='w+t') as f:
        f.write(file_content)
        f.flush()
        tasks = []
        for instance in instances:
            addr = get_rsync_addr(ctx, instance)
            task = sh.rsync(*utils.rsync_options(ctx, [f'--chmod={permissions}', f'--owner={user}']),
                            f.name,
                            f'{user}@{addr}:{file_path}',
                            _bg=True,
                            _timeout=60)
            tasks.append((instance, task))
        for name, run in tasks:
            try:
                run.wait()
            except(sh.TimeoutException, sh.ErrorReturnCode):
                _, out, err = str(run.exit_code), str(run.stdout), str(run.stderr)
                raise DataprocException(
                    f'Failed to create file {name}:{file_path}, out: {out}, err: {err}',
                    exc_info=True)
