#!/usr/bin/env python3

import logging
import os.path
import sys
import re
import paramiko
import socket
import psycopg2
import traceback
from json import load
import subprocess
from tenacity import retry, stop_after_attempt, wait_fixed
from contextlib import closing


CONNSTRING = 'dbname=postgres user=monitor connect_timeout=1 host=localhost'
UNNATURAL_PORT = 7432
PREV_PG_VERSIONS = {
    '11': '10',
    '12': '11',
    '13': '12',
    '14': '13',
}
BEFORE_MASTER_UPDATING_COMMANDS = {
    '11': [
        "sed -i '/replacement_sort_tuples/d' /var/lib/postgresql/{version_from}/data/conf.d/postgresql.conf",
    ],
    '12': [
        """
        for db in $(sudo -u postgres /usr/lib/postgresql/{version_to}/bin/psql
                -XtA -c "select datname from pg_database where datname <> 'template0'"); do
            for extension in $(sudo -u postgres /usr/lib/postgresql/{version_to}/bin/psql
                    -XtA -c "select extname from pg_extension where extname like '%postgis%'
                                                                 or extname = 'address_standardizer'" -d $db); do
                sudo -u postgres /usr/lib/postgresql/{version_to}/bin/psql
                        -XtA -c 'alter extension '$extension' update to "3.0.0"' -d $db;
            done;
        done
        """,
        # otherwise it won't work serial upgrade from PG 10 to PG 12+
        """cp /usr/lib/postgresql/{version_to}/lib/postgis_raster-3.so
              /usr/lib/postgresql/{version_to}/lib/rtpostgis-2.5""",
    ],
    '13': [
        """
        for db in $(sudo -u postgres /usr/lib/postgresql/{version_to}/bin/psql
                -XtA -c "select datname from pg_database where datname <> 'template0'"); do
            for extension in $(sudo -u postgres /usr/lib/postgresql/{version_to}/bin/psql
                    -XtA -c "select extname from pg_extension where extname like '%pgrouting%'" -d $db); do
                sudo -u postgres /usr/lib/postgresql/{version_to}/bin/psql
                        -XtA -c 'alter extension '$extension' update to "3.0.2"' -d $db;
            done;
        done
        """,
        "sed -i '/wal_keep_segments/d' /var/lib/postgresql/{version_from}/data/conf.d/postgresql.conf",
        """cp /usr/lib/postgresql/{version_to}/lib/postgis_raster-3.so
              /usr/lib/postgresql/{version_to}/lib/rtpostgis-2.5""",
    ],
    '14': [
        "sed -i '/operator_precedence_warning/d' /var/lib/postgresql/{version_from}/data/conf.d/postgresql.conf",
        """
        for db in $(sudo -u postgres /usr/lib/postgresql/{version_to}/bin/psql
                    -XtA -c "select datname from pg_database where datname <> 'template0'"); do
                sudo -u postgres /usr/lib/postgresql/{version_to}/bin/psql -XtA -c
                    "drop extension if exists pg_repack;
                     drop extension if exists mdb_perf_diag;
                     drop extension if exists heapcheck;" -d $db;
        done
        """,
        """
        grep mdb_perf_diag /var/lib/postgresql/{version_from}/data/conf.d/postgresql.conf
            && sed -i -e 's/,mdb_perf_diag//g' /var/lib/postgresql/{version_from}/data/conf.d/postgresql.conf
            || true
        """,
        """cp /usr/lib/postgresql/{version_to}/lib/postgis_raster-3.so
              /usr/lib/postgresql/{version_to}/lib/rtpostgis-2.5""",
    ],
}
AFTER_REPLICA_UPDATING_COMMANDS = {
    '12': [
        "sed -i '/standby_mode/d' /var/lib/postgresql/{version_to}/data/recovery.conf",
        """mv /var/lib/postgresql/{version_to}/data/recovery.conf
              /var/lib/postgresql/{version_to}/data/conf.d/recovery.conf""",
        """echo "include_if_exists = 'recovery.conf'" >>
              /var/lib/postgresql/{version_to}/data/conf.d/postgresql.conf""",
    ],
}
DROPABLE_EXTENSIONS = ['logerrors', 'pg_stat_kcache']


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


@retry(stop=stop_after_attempt(3), wait=wait_fixed(30), reraise=True)
def get_ssh_conn(host, ssh=None):
    """
    Get ssh conn with host. Reconnect if ssh is provided.
    """
    logging.info('Connect to %s' % host)
    if ssh is None:
        ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(hostname=host, username='root', timeout=3, banner_timeout=10)
    session = ssh.get_transport().open_session()
    paramiko.agent.AgentRequestHandler(session)
    session.get_pty()
    session.exec_command('tty')
    return ssh


def parse_consistency_checks(stdout):
    try:
        error = stdout[stdout.rfind(' ok') :]
        lines = error[error.find('\n') + 1 :].split('\n')
        if lines[1]:
            return lines[1]
        else:
            return lines[0][:60].strip() + ': ' + lines[0][60:].strip()
    except Exception:
        return None


def get_sftp_conn(conn):
    return paramiko.SFTPClient.from_transport(conn.get_transport())


def exec_command(fqdn, ssh, cmd, allow_fail=False):
    logging.info('Executing on %s: %s', fqdn, cmd)
    stdout, stderr = None, None

    for attempt in range(1, 4):
        try:
            _, stdout, stderr = ssh.exec_command(cmd)  # noqa
            break
        except (TimeoutError, paramiko.ssh_exception.SSHException):
            logging.warning('Attempt %d, reconnect', attempt)
            ssh = get_ssh_conn(fqdn, ssh)

    if stdout is None:
        raise RuntimeError('Cannot connect to %s' % fqdn)

    status = stdout.channel.recv_exit_status()
    if status != 0 and not allow_fail:
        message = f'Running {cmd} on {fqdn} failed with {status}.\n' f'Stdout: {stdout.read()}\nStderr: {stderr.read()}'
        logging.error(message)
        raise RuntimeError(message)
    return status == 0, stdout.read(), stderr.read()


def run_local(cmd, allow_fail=False):
    logging.info('Executing locally: %s', cmd)
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    stdout = stdout.decode()
    stderr = stderr.decode()
    status = proc.poll()
    if status != 0 and not allow_fail:
        message = f'Running {cmd} locally failed with {status}.\n' f'Stdout: {stdout}\nStderr: {stderr}'
        logging.error(message)
        raise RuntimeError(message)
    if stdout:
        logging.info('stdout: %s' % stdout)
    if stderr:
        logging.info('stderr: %s' % stderr)
    return status == 0, stdout, stderr


def check_is_master():
    try:
        with psycopg2.connect(CONNSTRING) as conn:
            cur = conn.cursor()
            cur.execute('SELECT pg_is_in_recovery()')
            if cur.fetchone()[0] is False:
                return True
    except Exception:
        return False


def check_version_is_correct(target_version):
    with closing(psycopg2.connect(CONNSTRING)) as conn:
        with closing(conn.cursor()) as cur:
            cur.execute("SELECT version()")
            (row_version,) = cur.fetchall()[0]
            match = re.search(r'PostgreSQL ([0-9]+\.[0-9]+) \(?[a-zA-Z]+ (.+?)\).+', row_version)
            if not match:
                print('Could not parse pg version')
                return False

            version, package = match.groups()
            curr_version_major = version.split('.')[0]
            if curr_version_major == target_version:
                print('No upgrade needed')
                return False

            prev_version = PREV_PG_VERSIONS[target_version]
            if prev_version != curr_version_major:
                print('Upgrade path does not exists')
                return False
    return True


def get_database_names():
    with closing(psycopg2.connect(CONNSTRING)) as conn:
        with closing(conn.cursor()) as cur:
            cur.execute("SELECT datname FROM pg_database WHERE datname <> 'template0'")
            return [row[0] for row in cur.fetchall()]


class UpgradeCheckFailed(Exception):
    """
    pg_upgrade --check returns error
    """


class PostgreSQLClusterUpgrade:
    def __init__(self, version_to, host_addrs):
        self.VERSION_TO = version_to
        self.VERSION_FROM = PREV_PG_VERSIONS[version_to]
        self.BEFORE_MASTER_UPDATING_COMMANDS = BEFORE_MASTER_UPDATING_COMMANDS.get(version_to, [])
        self.AFTER_REPLICA_UPDATING_COMMANDS = AFTER_REPLICA_UPDATING_COMMANDS.get(version_to, [])
        self.host_addrs = host_addrs
        self.changes = []
        self.user_exposable_error = ''

    def install_logging(self):
        logging.basicConfig(
            filename='/var/log/postgresql/upgrade_%s.log' % self.VERSION_TO,
            level=logging.DEBUG,
            format='%(asctime)s [%(levelname)s] %(message)s',
        )
        paramiko.util.logging.getLogger().setLevel(logging.INFO)

    def _get_exe_func(self, fqdn=None):
        def exe(cmd, context=None, allow_fail=False):
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
            if fqdn is None:
                return run_local(result_cmd, allow_fail=allow_fail)
            else:
                return exec_command(fqdn, self.ssh_conns[fqdn], result_cmd, allow_fail=allow_fail)  # noqa

        return exe

    def _open_ssh_conns(self, hosts):
        self.ssh_conns = {}
        for host in hosts:
            self.ssh_conns[host] = get_ssh_conn(self.host_addrs.get(host, host))

    def _shutdown(self, fqdn, version, port=5432):
        exe = self._get_exe_func(fqdn)

        exe('service odyssey stop || service pgbouncer stop', allow_fail=True)

        res, *_ = exe(
            'sudo -u postgres /usr/local/yandex/pg_wait_started.py -w 3 -m {version} --port {port}',
            context={'version': version, 'port': port},
            allow_fail=True,
        )
        if res:
            exe(
                'sudo -u postgres /usr/lib/postgresql/{version}/bin/pg_ctl -m fast stop -t 3600 '
                '-D /etc/postgresql/{version}/data '
                "-o ' -c port={port}'",
                context={'version': version, 'port': port},
            )
            exe('rm -f /var/lib/postgresql/{version}/data/postmaster.pid', context={'version': version})

    def _check_pg_dump_fail_case(self):
        # returns True if impossible to do pg_dump because of low max_locks_per_transaction, see MDB-7393
        exe = self._get_exe_func()
        _, stdout, _ = exe(
            'for db in $(sudo -u postgres /usr/lib/postgresql/{version_from}/bin/psql '
            '-XtA -c "select datname from pg_database where datname <> '
            "'template0'\"); do sudo -u postgres /usr/lib/postgresql/{version_from}/bin/psql -XtA "
            "-c \"SELECT (SELECT COUNT(1) FROM pg_class WHERE relkind IN ('r','p'))/current_setting('max_connections')::int > "
            "current_setting('max_locks_per_transaction')::int\" -d $db; done",
            allow_fail=True,
        )
        if 't' in stdout.split('\n'):
            print('Upgrade failed, you might need to increase max_locks_per_transaction')
            sys.exit(3)

    def _check_aggregates_case(self):
        if self.VERSION_TO != '14':
            return

        errors = []
        for dbname in get_database_names():
            dbconnstring = f'dbname={dbname} user=monitor connect_timeout=1 host=localhost'
            with closing(psycopg2.connect(dbconnstring)) as conn:
                with closing(conn.cursor()) as cur:
                    cur.execute(
                        """
                        SELECT 'aggregate' AS objkind, p.oid::regprocedure::text AS objname
                            FROM pg_proc AS p
                            JOIN pg_aggregate AS a ON a.aggfnoid=p.oid
                            JOIN pg_proc AS transfn ON transfn.oid=a.aggtransfn
                            WHERE p.oid >= 16384
                            AND a.aggtransfn = ANY(ARRAY['array_position(anyarray,anyelement)','array_position(anyarray,anyelement,integer)','array_positions(anyarray,anyelement)','width_bucket(anyelement,anyarray)', 'array_append(anyarray,anyelement)','array_cat(anyarray,anyarray)','array_prepend(anyelement,anyarray)']::regprocedure[])
                            AND not(p.oid::regprocedure::text ~ 'repack')
                        UNION ALL
                        SELECT 'aggregate' AS objkind, p.oid::regprocedure::text AS objname
                            FROM pg_proc AS p
                            JOIN pg_aggregate AS a ON a.aggfnoid=p.oid
                            JOIN pg_proc AS finalfn ON finalfn.oid=a.aggfinalfn
                            WHERE p.oid >= 16384
                            AND a.aggfinalfn = ANY(ARRAY['array_position(anyarray,anyelement)','array_position(anyarray,anyelement,integer)','array_positions(anyarray,anyelement)','width_bucket(anyelement,anyarray)', 'array_append(anyarray,anyelement)','array_cat(anyarray,anyarray)','array_prepend(anyelement,anyarray)']::regprocedure[])
                        UNION ALL
                        SELECT 'operator' AS objkind, op.oid::regoperator::text AS objname
                            FROM pg_operator AS op
                            WHERE op.oid >= 16384
                            AND oprcode = ANY(ARRAY['array_position(anyarray,anyelement)','array_position(anyarray,anyelement,integer)','array_positions(anyarray,anyelement)','width_bucket(anyelement,anyarray)', 'array_append(anyarray,anyelement)','array_cat(anyarray,anyarray)','array_prepend(anyelement,anyarray)']::regprocedure[])
                    """
                    )
                    for objkind, objname in cur.fetchall():
                        errors.append(f'{objkind} {objname} in {dbname}')
        if errors:
            print('Drop these objects and recreate after upgrade: ' + ', '.join(errors))
            sys.exit(3)

    def _upgrade_master(self):
        exe = self._get_exe_func()

        self.changes.append('pg_reset_autoconf')

        for cmd in self.BEFORE_MASTER_UPDATING_COMMANDS:
            exe(cmd)
        if self.VERSION_FROM >= '12':
            # because MDB-18207 (recovery.conf could be not empty after switchover)
            exe("rm -f /var/lib/postgresql/{version_from}/data/conf.d/recovery.conf")

        collate = exe(
            'sudo -u postgres /usr/lib/postgresql/{version_from}/bin/psql '
            '-tAX -c "select datcollate from pg_database where '
            "datname = 'postgres';\""
        )[1].split()[0]
        ctype = exe(
            'sudo -u postgres /usr/lib/postgresql/{version_from}/bin/psql '
            '-tAX -c "select datctype from pg_database where '
            "datname = 'postgres';\""
        )[1].split()[0]

        exe('sudo -u postgres /usr/lib/postgresql/{version_from}/bin/psql -c ' "'ALTER SYSTEM RESET ALL;'")
        exe(
            'sudo -u postgres /usr/lib/postgresql/{version_from}/bin/psql -c '
            "\"ALTER SYSTEM SET autovacuum TO 'off'\""
        )
        exe(
            'sudo -u postgres /usr/lib/postgresql/{version_from}/bin/psql -c '
            "\"ALTER SYSTEM SET archive_command TO '/bin/false'\""
        )
        exe('sudo -u postgres /usr/lib/postgresql/{version_from}/bin/psql -c ' "'select pg_reload_conf();'")

        self.changes.append('shutdown_old_version')
        self._shutdown(None, version=self.VERSION_FROM)

        checksums, *_ = exe(
            'sudo -u postgres /usr/lib/postgresql/{version_from}/bin/'
            'pg_controldata /var/lib/postgresql/{version_from}/data | '
            "grep 'Data page checksum version' | grep 1",
            allow_fail=True,
        )

        self.changes.append('mkdir_etc_postgresql')
        exe('rsync -a --delete /etc/postgresql/{version_from}/ /etc/postgresql/{version_to}/')
        exe(
            r"sed -i -e 's/{version_from_escaped}/{version_to_escaped}/g' "
            '/etc/postgresql/{version_to}/data/postgresql.conf'
        )

        self.changes.append('mkdir_var_lib_postgresql')
        exe('sudo -u postgres mkdir -p /var/lib/postgresql/{version_to}/data')
        exe(
            'sudo -u postgres /usr/lib/postgresql/{version_to}/bin/'
            'initdb {checksums_flag} -D /var/lib/postgresql/{version_to}/data '
            '--encoding=UTF8 --locale=en_US.UTF-8 '
            '--pwfile=/var/lib/postgresql/.pwfile '
            '--lc-collate={collate} --lc-ctype={ctype}',
            context={
                'checksums_flag': '-k' if checksums else '',
                'collate': collate,
                'ctype': ctype,
            },
        )
        exe('cp -ar /var/lib/postgresql/{version_from}/data/conf.d/ /var/lib/postgresql/{version_to}/data/conf.d/')
        exe(
            r"sed -i -e 's/\/{version_from_escaped}/\/{version_to_escaped}/g' -e '/sql_inheritance/d' "
            '/var/lib/postgresql/{version_to}/data/conf.d/postgresql.conf'
        )
        exe(r'sed -i -e "s/^localhost/*/g" /var/lib/postgresql/.pgpass')

        is_ok, stdout, _ = exe(
            'cd /var/lib/postgresql && sudo -u postgres '
            '/usr/lib/postgresql/{version_to}/bin/pg_upgrade '
            '-b /usr/lib/postgresql/{version_from}/bin/ '
            '-B /usr/lib/postgresql/{version_to}/bin/ '
            '-d /var/lib/postgresql/{version_from}/data/ '
            '-D /var/lib/postgresql/{version_to}/data/ '
            "-o ' -c config_file=/etc/postgresql/{version_from}/data/postgresql.conf' "
            "-O ' -c config_file=/etc/postgresql/{version_to}/data/postgresql.conf' "
            '--link --check',
            allow_fail=True,
        )
        if not is_ok:
            error_message = parse_consistency_checks(stdout)
            filenames = [line.strip() for line in stdout.split('\n') if line.endswith('.txt')]
            additional_info = []
            for filename in filenames:
                _, content, _ = exe('cat /var/lib/postgresql/' + filename, allow_fail=True)
                if content:
                    additional_info.append(content.strip())
            if additional_info:
                error_message = error_message + ' (%s)' % '; '.join(additional_info)
            self.user_exposable_error = error_message
            raise UpgradeCheckFailed

        # The point of no return
        self.changes.append('pg_upgrade')
        exe(
            'cd /var/lib/postgresql && sudo -u postgres '
            '/usr/lib/postgresql/{version_to}/bin/pg_upgrade '
            '-b /usr/lib/postgresql/{version_from}/bin/ '
            '-B /usr/lib/postgresql/{version_to}/bin/ '
            '-d /var/lib/postgresql/{version_from}/data/ '
            '-D /var/lib/postgresql/{version_to}/data/ '
            "-o ' -c config_file=/etc/postgresql/{version_from}/data/postgresql.conf' "
            "-O ' -c config_file=/etc/postgresql/{version_to}/data/postgresql.conf' "
            '--link'
        )
        exe('mv /etc/postgresql/{version_from}/data /tmp/etc-{version_from}-data-bak')
        exe(
            'cat /var/lib/postgresql/{version_from}/data/postgresql.auto.conf > '
            '/var/lib/postgresql/{version_to}/data/postgresql.auto.conf'
        )
        res, *_ = exe('ls /etc/wal-g/envdir/WALE_S3_PREFIX', allow_fail=True)
        if res:
            exe(r'sed -i -e "sq/{version_from_num}/\$q/{version_to_num}/q" /etc/wal-g/envdir/WALE_S3_PREFIX')
        else:
            exe(
                r'old_one=`cat /etc/wal-g/wal-g.yaml | grep _S3_PREFIX` && '
                r'new_one=`echo -n "$old_one" | sed -e "s|/{version_from_num}/|/{version_to_num}/|"` && '
                r'sed -i -e "s|$old_one|$new_one|" /etc/wal-g/wal-g.yaml'
            )

        exe(
            'sudo -u postgres /usr/lib/postgresql/{version_to}/bin/pg_ctl start '
            '-D /var/lib/postgresql/{version_to}/data/ '
            "-o ' -c config_file=/etc/postgresql/{version_to}/data/postgresql.conf' "
            "-o ' -c port={port}'",
            context={'port': UNNATURAL_PORT},
            allow_fail=True,
        )
        exe(
            'sudo -u postgres /usr/local/yandex/pg_wait_started.py -w 1800 -m {version_to} --port {port}',
            context={'port': UNNATURAL_PORT},
            allow_fail=True,
        )
        exe(
            '/usr/bin/timeout 300 '
            'sudo -u postgres /usr/lib/postgresql/{version_to}/bin/vacuumdb --port {port} '
            '--analyze-in-stages --all -j 8',
            context={'port': UNNATURAL_PORT},
            allow_fail=True,
        )
        self._shutdown(None, version=self.VERSION_TO, port=UNNATURAL_PORT)

    def rollback(self):
        exe = self._get_exe_func()

        if 'pg_upgrade' in self.changes:
            return False

        if 'mkdir_var_lib_postgresql' in self.changes:
            exe('rm -rf /var/lib/postgresql/{version_to}')
        if 'mkdir_etc_postgresql' in self.changes:
            exe('rm -rf /etc/postgresql/{version_to}')
        if 'shutdown_old_version' in self.changes:
            self._open_master(version=self.VERSION_FROM)
        if 'pg_reset_autoconf' in self.changes:
            exe('sudo -u postgres /usr/lib/postgresql/{version_from}/bin/psql -c ' "'ALTER SYSTEM RESET ALL;'")
            exe('sudo -u postgres /usr/lib/postgresql/{version_from}/bin/psql -c ' "'select pg_reload_conf();'")
        return True

    def _upgrade_replica(self, replica_fqdn):
        self._shutdown(replica_fqdn, version=self.VERSION_FROM)

        master_exec = self._get_exe_func()
        master_exec(
            'cd /var/lib/postgresql && rsync --relative --archive '
            '--exclude={version_from}/data/pg_xlog --exclude={version_from}/data/conf.d/recovery.conf '
            '--hard-links --size-only --no-inc-recursive {version_from}/data {version_to}/data root@[{replica}]:'
            '/var/lib/postgresql',
            {'replica': self.host_addrs.get(replica_fqdn, replica_fqdn)},
        )

        exe = self._get_exe_func(replica_fqdn)
        exe('rsync -a --delete /etc/postgresql/{version_from}/ /etc/postgresql/{version_to}/')
        exe('mv /etc/postgresql/{version_from}/data /tmp/etc-{version_from}-data-bak')
        exe(
            r"sed -i -e 's/{version_from_escaped}/{version_to_escaped}/g' "
            '/etc/postgresql/{version_to}/data/postgresql.conf'
        )
        res, *_ = exe('ls /var/lib/postgresql/{version_from}/data/recovery.conf', allow_fail=True)
        if res:
            exe(
                'cp -a /var/lib/postgresql/{version_from}/data/recovery.conf '
                '/var/lib/postgresql/{version_to}/data/recovery.conf'
            )
        else:
            exe(
                'cp -a /var/lib/postgresql/{version_from}/data/conf.d/recovery.conf '
                '/var/lib/postgresql/{version_to}/data/conf.d/recovery.conf'
            )
        res, *_ = exe('ls /etc/wal-g/envdir/WALE_S3_PREFIX', allow_fail=True)
        if res:
            exe(r'sed -i -e "sq/{version_from_num}/\$q/{version_to_num}/q" /etc/wal-g/envdir/WALE_S3_PREFIX')
        else:
            exe(
                r'old_one=`cat /etc/wal-g/wal-g.yaml | grep _S3_PREFIX` && '
                r'new_one=`echo -n "$old_one" | sed -e "s|/{version_from_num}/|/{version_to_num}/|"` && '
                r'sed -i -e "s|$old_one|$new_one|" /etc/wal-g/wal-g.yaml'
            )

        for cmd in self.AFTER_REPLICA_UPDATING_COMMANDS:
            exe(cmd)
        if self.VERSION_TO >= '12':
            exe("sudo -u postgres touch /var/lib/postgresql/{version_to}/data/standby.signal")

        exe('service postgresql@{version_to}-data start')
        exe('sudo -u postgres /usr/local/yandex/pg_wait_started.py -w 1800 -m {version_to}')
        exe('service pgbouncer start || service odyssey start', allow_fail=True)

    def _open_master(self, version):
        exe = self._get_exe_func()

        exe('service postgresql@{version}-data start', context={'version': version})
        exe('sudo -u postgres /usr/local/yandex/pg_wait_started.py -w 1800 -m {version}', context={'version': version})
        exe(
            'sudo -u postgres /usr/lib/postgresql/{version}/bin/psql -c ' "'vacuum verbose repl_mon;'",
            context={'version': version},
        )
        exe('service pgbouncer start || service odyssey start', allow_fail=True)
        exe('service ferm status && service ferm restart', allow_fail=True)

    def _prepare_for_highstate(self, fqdn):
        exe = self._get_exe_func(fqdn)
        exe('sudo -u postgres /usr/lib/postgresql/{version_to}/bin/psql -c ' "'ALTER SYSTEM RESET ALL;'")
        exe('sudo -u postgres /usr/lib/postgresql/{version_to}/bin/psql -c ' "'select pg_reload_conf();'")

        if fqdn is None:  # it's master
            exe(
                'nohup sudo -u postgres /usr/lib/postgresql/{version_to}/bin/vacuumdb '
                '--analyze --all -j 2 >/dev/null 2>&1 &'
            )
            return

        if self.VERSION_FROM < '12':
            recovery_conf = '/var/lib/postgresql/{version_from}/data/recovery.conf'
        else:
            recovery_conf = '/var/lib/postgresql/{version_from}/data/conf.d/recovery.conf'
        check_recovery_conf_cmd = 'ls {recovery_conf}'.format(recovery_conf=recovery_conf)
        res, *_ = exe(check_recovery_conf_cmd, allow_fail=True)
        if res:
            stream_from = None
            slot_name = None

            filepath = recovery_conf.format(version_from=self.VERSION_FROM)
            if fqdn:
                sftp = get_sftp_conn(self.ssh_conns[fqdn])
                with sftp.open(filepath, 'r') as fobj:
                    lines = fobj.readlines()  # pylint: disable=no-member
            else:
                with open(filepath, 'r') as fobj:
                    lines = fobj.readlines()  # pylint: disable=no-member
            for line in lines:
                if line.startswith('primary_conninfo = '):
                    for token in line.split("'")[1].split():
                        if token.startswith('host='):
                            stream_from = token.split('=')[1]
                elif line.startswith('primary_slot_name = '):
                    slot_name = line.split("'")[1]
            if slot_name and stream_from:
                exe(
                    "sudo -u postgres psql 'host={stream_from} port=5432 user=repl replication=true' -c "
                    '"CREATE_REPLICATION_SLOT {slot_name} PHYSICAL;"',
                    {
                        'stream_from': stream_from,
                        'slot_name': slot_name,
                    },
                )

    def _update_extensions(self):
        exe = self._get_exe_func()
        exe(
            """
            psql='sudo -u postgres /usr/lib/postgresql/{version_to}/bin/psql';
            for db in $($psql -XtA -c "SELECT datname FROM pg_database WHERE datname <> 'template0'"); do
              for extension in $($psql -XtA -c "SELECT extname FROM pg_extension" -d "$db"); do
                if [ "$extension" == 'pg_repack' ]; then
                  $psql -XtA -c "DROP EXTENSION pg_repack CASCADE; CREATE EXTENSION pg_repack" -d "$db";
                else
                  $psql -XtA -c "ALTER extension \\"$extension\\" UPDATE" -d "$db" || true;
                fi
              done
            done
        """
        )

    def _drop_extensions(self):
        exe = self._get_exe_func()
        for extension in DROPABLE_EXTENSIONS:
            exe(
                'for db in $(sudo -u postgres /usr/lib/postgresql/{version_to}/bin/psql '
                '-XtA -c "select datname from pg_database where datname <> '
                "'template0'\"); do sudo -u postgres "
                '/usr/lib/postgresql/{version_to}/bin/psql -XtA -c '
                '"drop extension if exists {extension};" -d $db; done'.format(
                    version_to=self.VERSION_TO, extension=extension
                )
            )

    def _check_master_replicas_ssh(self, replicas):
        master_exec = self._get_exe_func()
        for replica in replicas:
            master_exec('ssh root@{replica} /bin/true', {'replica': self.host_addrs.get(replica, replica)})

    def run(self, replicas):
        self.install_logging()

        try:
            self._check_pg_dump_fail_case()
            self._check_aggregates_case()

            self._open_ssh_conns(replicas)
            self._check_master_replicas_ssh(replicas)

            self._drop_extensions()
            self._update_extensions()
        except Exception:
            sys.exit(4)

        try:
            self._upgrade_master()
        except Exception:
            for line in traceback.format_exc().split('\n'):
                print(line.rstrip(), file=sys.stderr)
            try:
                rolled_back = self.rollback()
                if rolled_back:
                    if self.user_exposable_error:
                        print(self.user_exposable_error)
                        sys.exit(3)
                    sys.exit(4)
            except Exception:
                for line in traceback.format_exc().split('\n'):
                    print(line.rstrip(), file=sys.stderr)
            sys.exit(1)

        try:
            for replica in replicas:
                self._upgrade_replica(replica)
            self._open_master(version=self.VERSION_TO)
            self._prepare_for_highstate(None)
            for replica in replicas:
                self._prepare_for_highstate(replica)
            self._update_extensions()
        except Exception:
            for line in traceback.format_exc().split('\n'):
                print(line.rstrip(), file=sys.stderr)
            sys.exit(1)


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Usage: ' + sys.argv[0] + ' <PG version to>')
        sys.exit(4)

    pg_version_to = sys.argv[1]

    if pg_version_to not in PREV_PG_VERSIONS:
        print('PG version %s not found, possible: 11, 12, 13, 14' % pg_version_to)
        sys.exit(4)

    if not check_is_master():
        print('I am not master, exit.')
        sys.exit(4)

    if not check_version_is_correct(pg_version_to):
        sys.exit(4)

    with open('/etc/dbaas.conf', 'r') as dbaas_conf:
        cluster_hosts = load(dbaas_conf)['cluster_hosts']

    master = socket.gethostname()
    replicas = [h for h in cluster_hosts if h != master]

    cluster_addrs = {}
    if os.path.exists('/tmp/cluster_node_addrs.conf'):
        with open('/tmp/cluster_node_addrs.conf', 'r') as cluster_addrs:
            cluster_addrs = load(cluster_addrs)

    PostgreSQLClusterUpgrade(pg_version_to, cluster_addrs).run(replicas)
