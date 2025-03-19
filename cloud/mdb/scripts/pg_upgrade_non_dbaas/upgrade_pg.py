#!/usr/bin/env python3

import logging
import sys

import paramiko
from kazoo.client import KazooClient
from kazoo.exceptions import NoNodeError
from retrying import retry as retry_dec

logging.basicConfig(level=logging.DEBUG, format='%(asctime)s [%(levelname)s] %(message)s')

paramiko.util.logging.getLogger().setLevel(logging.INFO)

LOG = logging.getLogger('main')


def get_major_version_num(version):
    """
    major_num in pillar
    Examples: 9.6 -> 906; 10 -> 1000
    """
    if '.' in version:
        major, minor = version.split('.')
    else:
        major = version
        minor = 0
    return str(100 * int(major) + int(minor))


@retry_dec(stop_max_attempt_number=3, wait_random_min=1000, wait_random_max=10000)
def get_ssh_conn(host):
    """
    Get ssh conn with host
    """
    LOG.info('Connect to %s' % host)
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(hostname=host, username='root', timeout=1, banner_timeout=1)
    session = ssh.get_transport().open_session()
    paramiko.agent.AgentRequestHandler(session)
    session.get_pty()
    session.exec_command('tty')
    return ssh


def get_sftp_conn(conn):
    return paramiko.SFTPClient.from_transport(conn.get_transport())


def exec_command(fqdn, ssh, cmd, env=None, allow_fail=False):
    LOG.info('Executing %s on %s', cmd, fqdn)
    _, stdout, stderr = ssh.exec_command(cmd, environment=env)  # noqa
    status = stdout.channel.recv_exit_status()
    if status != 0 and not allow_fail:
        message = f'Running {cmd} on {fqdn} failed with {status}.\n' f'Stdout: {stdout.read()}\nStderr: {stderr.read()}'
        LOG.error(message)
        raise RuntimeError(message)
    return status, stdout.read(), stderr.read()


class PostgreSQLClusterUpgrade:
    """
    Abstract class for upgrading PostgreSQL.
    """

    VERSION_FROM = ""
    VERSION_TO = ""

    def __init__(self, host):
        self.host = host

    def _get_exe_func(self, fqdn, conn=None):
        def exe(cmd, context=None, env=None, allow_fail=False):
            if context is None:
                context = {}
            context.update(
                {
                    'version_from': self.VERSION_FROM,
                    'version_to': self.VERSION_TO,
                    'version_from_escaped': self.VERSION_FROM.replace('.', r'\.'),
                    'version_to_escaped': self.VERSION_TO.replace('.', r'\.'),
                    'version_from_num': get_major_version_num(self.VERSION_FROM),
                    'version_to_num': get_major_version_num(self.VERSION_TO),
                }
            )
            result_cmd = cmd.format(**context).replace('\n', ' ')
            return exec_command(fqdn, conn or self.ssh_conns[fqdn], result_cmd, env=env, allow_fail=allow_fail)  # noqa

        return exe

    def _open_ssh_conns(self, hosts):
        self.ssh_conns = {}
        for host in hosts:
            self.ssh_conns[host] = get_ssh_conn(host)
        return self.ssh_conns

    def _install_pkgs(self, fqdn):
        exe = self._get_exe_func(fqdn)
        exe("salt-call state.sls components.postgresql_cluster.operations.install-packages saltenv=dev")

    def _upgrade_cluster(self, fqdn):
        exe = self._get_exe_func(fqdn)
        return exe('salt-call mdb_postgresql.upgrade {version_to}')

    def _cleanup(self, fqdn, master_fqdn):
        exe = self._get_exe_func(fqdn)
        if fqdn == master_fqdn:
            exe(
                "salt-call state.sls components.postgresql_cluster.operations.cleanup-after-upgrade pillar=\'{pillar}\' saltenv=dev",
                context={'pillar': {'data': {'version_from': self.VERSION_FROM}}},
            )
        else:
            exe(
                "sed -i '/{master_fqdn}/d' {path}",
                context={'master_fqdn': master_fqdn, 'path': '/root/.ssh/authorized_keys2'},
            )

    def _set_zk_hosts_and_prefix(self, fqdn, conn):
        exe = self._get_exe_func(fqdn, conn)
        self.zk_hosts = exe('grep zk_hosts /etc/pgsync.conf')[1].decode('utf-8').split()[2]
        self.zk_lockpath_prefix = exe('grep zk_lockpath_prefix /etc/pgsync.conf')[1].decode('utf-8').split()[2]

    def _get_master_and_replicas(self):
        zk = KazooClient(self.zk_hosts)
        zk.start()
        all_hosts = zk.get_children(self.zk_lockpath_prefix + '/all_hosts')
        try:
            leader_lock = zk.get_children(self.zk_lockpath_prefix + '/leader')[0]
            master, _ = zk.get(self.zk_lockpath_prefix + '/leader/' + leader_lock)
            master = master.decode('utf-8')
        except NoNodeError:
            master = self.host
        except IndexError:
            master = self.host
        replicas = [h for h in all_hosts if h != master]
        zk.stop()
        return master, replicas

    def _pgsync_maintenance(self, command):
        zk = KazooClient(self.zk_hosts)
        zk.start()
        zk.ensure_path(self.zk_lockpath_prefix + '/maintenance')
        zk.set(self.zk_lockpath_prefix + '/maintenance', command.encode('utf-8'))
        zk.stop()

    def _pgsync_absent_timeline(self):
        zk = KazooClient(self.zk_hosts)
        zk.start()
        zk.delete(self.zk_lockpath_prefix + '/timeline', recursive=True)
        zk.stop()

    def install_ssh_keys(self, master, replicas):
        exe = self._get_exe_func(master)
        _, stdout, _ = exe("salt-call mdb_postgresql.generate_ssh_keys")
        public = stdout.decode('utf-8').split('\n')[1].strip()
        for replica in replicas:
            self._get_exe_func(replica)(f"echo '{public}' >> /root/.ssh/authorized_keys2")

    def run(self):
        self._set_zk_hosts_and_prefix(self.host, get_ssh_conn(self.host))
        master, replicas = self._get_master_and_replicas()
        self._open_ssh_conns([master] + replicas)

        if replicas:
            self.install_ssh_keys(master, replicas)

        for fqdn in self.ssh_conns.keys():
            self._install_pkgs(fqdn)
        self._pgsync_maintenance('enable')

        status, stdout, stderr = self._upgrade_cluster(master)
        if status != 0:
            print(stdout)
            raise RuntimeError(stderr)

        stdout = stdout.decode('utf-8')
        is_upgraded = stdout[stdout.find('is_upgraded') :].split('\n')[1].strip()
        if is_upgraded != 'True':
            print(stdout)
            raise RuntimeError(stdout)

        self._pgsync_absent_timeline()

        for fqdn, conn in self.ssh_conns.items():
            pillar = {'service-restart': True} if fqdn != master and self.VERSION_TO == 12 else {}
            exec_command(fqdn, conn, "salt-call state.highstate queue=True pillar=\"{pillar}\"".format(pillar=pillar))

        self._pgsync_maintenance('disable')

        for fqdn, conn in self.ssh_conns.items():
            self._cleanup(fqdn, master)


class PostgreSQLClusterUpgrade11(PostgreSQLClusterUpgrade):
    """
    Upgrade postgresql cluster to version 11
    """

    VERSION_FROM = '10'
    VERSION_TO = '11'


class PostgreSQLClusterUpgrade12(PostgreSQLClusterUpgrade):
    """
    Upgrade postgresql cluster to version 12
    """

    VERSION_FROM = '11'
    VERSION_TO = '12'


class PostgreSQLClusterUpgrade13(PostgreSQLClusterUpgrade):
    """
    Upgrade postgresql cluster to version 13
    """

    VERSION_FROM = '12'
    VERSION_TO = '13'


class PostgreSQLClusterUpgrade14(PostgreSQLClusterUpgrade):
    """
    Upgrade postgresql cluster to version 14
    """

    VERSION_FROM = '13'
    VERSION_TO = '14'


def main():
    if len(sys.argv) != 3:
        print('Usage: ' + sys.argv[0] + '<master fqdn> <PG version to>')
        sys.exit(1)

    host = sys.argv[1]
    pg_version_to = sys.argv[2]

    upgrader = {
        '11': PostgreSQLClusterUpgrade11,
        '12': PostgreSQLClusterUpgrade12,
        '13': PostgreSQLClusterUpgrade13,
        '14': PostgreSQLClusterUpgrade14,
    }.get(pg_version_to)
    if not upgrader:
        print('PG version %s not found, possible: 11, 12, 13, 14' % pg_version_to)
        sys.exit(1)

    LOG.info('Upgrading %s', host)
    upgrader_instance = upgrader(host)
    upgrader_instance.run()


if __name__ == '__main__':
    main()
