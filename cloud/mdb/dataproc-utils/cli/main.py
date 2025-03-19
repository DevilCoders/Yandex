import os
import re
import copy
import json
import yaml
import psycopg2
import shlex
import socket
import subprocess
import sshtunnel
from psycopg2.extras import DictCursor
from click import group, option, ClickException

YAML_CONFIG_PATH = '~/mdb-scripts/config.yaml'
DBAAS_BIN_PATH = '~/mdb-scripts/bin/dbaas'


def load_yaml(file_path):
    with open(os.path.expanduser(file_path), 'r') as f:
        return yaml.safe_load(f)


def main():
    cli()


@group()
def cli():
    pass


@group('kafka')
def kafka_group():
    """Kafka helper commands."""
    pass


cli.add_command(kafka_group)


@kafka_group.command('fill_package_version')
@option('-n', '--dry-run', is_flag=True)
@option('--profile')
def fill_package_version_command(dry_run, profile):
    query = """
        SELECT c.cid, c.status, p.value #>> '{data,kafka,version}' AS version, p.value #>> '{data,kafka,package_version}' AS package
        FROM dbaas.clusters c
        JOIN dbaas.pillar p USING (cid)
        WHERE type = 'kafka_cluster' AND code.visible(c) AND p.value #>> '{data,kafka,package_version}' IS NULL;
    """

    rows = exec_query(profile, query)
    package_by_version = {
        '2.1': '2.1.1-java11',
        '2.6': '2.6.0-java11',
        '2.8': '2.8.0-java11',
    }

    if not rows:
        print('Package version is set for all Kafka clusters. Exiting.')
        return

    print(f"There are {len(rows)} Kafka clusters without package version set in pillar")
    updated = 0
    skipped = 0

    for row in rows:
        cid = row['cid']
        print(f'processing cluster {cid}')

        status = row['status']
        if status not in ('RUNNING', 'STOPPED'):
            print(f'  cluster is in state {status} and will be skipped')
            skipped += 1
            continue

        version = row['version']
        if row['package']:
            print('  package already filled')
            continue

        package_version = package_by_version.get(version)
        if not package_version:
            raise RuntimeError(f'unknown version {version}')

        if dry_run:
            print(f'  (dry run) package will be set to {package_version}')
        else:
            print(f'  package will be set to {package_version}')
            updated += 1
            dbaas_path = os.path.expanduser(DBAAS_BIN_PATH)
            command = f"{dbaas_path} --profile {profile} pillar set-key -c {cid} -- data:kafka:package_version '{package_version}'"
            output = subprocess.check_output(shlex.split(command))
            print(f'  {output.decode("utf-8").strip()}')

    print(f'Package version was set in {updated} clusters. {skipped} clusters were skipped due to running operation.')


@kafka_group.command('switch_py4j')
@option('-n', '--dry-run', is_flag=True)
@option('--profile')
@option('--env')
@option('--turn-off', is_flag=True)
def switch_py4j_command(dry_run, profile, env, turn_off):
    query = """
        SELECT c.cid, c.status, p.value #>> '{data,kafka,acls_via_py4j}' AS acls_via_py4j, p.value as pillar
        FROM dbaas.clusters c
        JOIN dbaas.pillar p USING (cid)
        WHERE type = 'kafka_cluster' AND code.visible(c) AND c.env = %(env)s;
    """

    rows = exec_query(profile, query, env=env)

    acls_via_py4j = not turn_off
    rows = [row for row in rows if bool(row['acls_via_py4j']) != acls_via_py4j]

    if not rows:
        print(f'Py4j state is {acls_via_py4j} for all Kafka clusters. Exiting.')
        return

    print(f"There are {len(rows)} Kafka clusters with py4j in state {not acls_via_py4j}")
    updated = 0
    skipped = 0
    fqdns = []

    for row in rows:
        cid = row['cid']
        print(f'processing cluster {cid}')

        status = row['status']
        if status not in ('RUNNING', 'STOPPED'):
            print(f'  cluster is in state {status} and will be skipped')
            skipped += 1
            continue

        users = row['pillar']['data']['kafka']['users']
        num_users = len(users)
        num_permissions = 0
        for user_name, user_data in users.items():
            num_permissions += len(user_data['permissions'])

        nodes = row['pillar']['data']['kafka']['nodes']
        fqdn = next(fqdn for fqdn, node in nodes.items() if node['id'] == 1)
        fqdn = fqdn.replace(".mdb.yandexcloud.net", ".db.yandex.net")
        fqdns.append(fqdn)
        print(f"  num users: {num_users}, num permissions: {num_permissions}, main broker: {fqdn}")

        if dry_run:
            print(f"  (dry run) data:kafka:acls_via_py4j will be set to {acls_via_py4j}")
        else:
            print(f"  data:kafka:acls_via_py4j will be set to {acls_via_py4j}")
            updated += 1
            dbaas_path = os.path.expanduser(DBAAS_BIN_PATH)
            flag = 'true' if acls_via_py4j else 'false'
            command = f"{dbaas_path} --profile {profile} pillar set-key -c {cid} -- data:kafka:acls_via_py4j {flag}"
            output = subprocess.check_output(shlex.split(command))
            print(f'  {output.decode("utf-8").strip()}')

    print(f'{updated} clusters were updated. {skipped} clusters were skipped due to running operation.')
    print(f"Main brokers: {','.join(fqdns)}")


@kafka_group.command('enable_old_jmx_user')
@option('-n', '--dry-run', is_flag=True)
@option('--profile')
@option('--env')
@option('--turn-on', is_flag=True)
def enable_old_jmx_user(dry_run, profile, env, turn_on):
    query = """
        SELECT c.cid, sc.subcid, c.status, p.value #>> '{data,kafka,old_jmx_user}' AS old_jmx_user, p.value as pillar
        FROM dbaas.clusters c
        JOIN dbaas.subclusters sc using (cid)
        LEFT JOIN dbaas.pillar p USING (subcid)
        WHERE type = 'kafka_cluster' AND code.visible(c) AND c.env = %(env)s AND sc.roles='{kafka_cluster}';
    """

    rows = exec_query(profile, query, env=env)

    old_jmx_user_value = turn_on
    rows = [row for row in rows if bool(row['old_jmx_user']) != old_jmx_user_value]

    if not rows:
        print(f'Old jmx user is {old_jmx_user_value} for all Kafka clusters. Exiting.')
        return

    print(f"There are {len(rows)} Kafka clusters with not old jmx user in state {not old_jmx_user_value}")
    updated = 0
    skipped = 0
    dbaas_path = os.path.expanduser(DBAAS_BIN_PATH)

    for row in rows:
        cid = row['cid']
        subcid = row['subcid']
        print(f'processing cluster {cid} subcid {subcid}')

        status = row['status']
        if status not in ('RUNNING', 'STOPPED'):
            print(f'  cluster is in state {status} and will be skipped')
            skipped += 1
            continue

        if dry_run:
            print(f"  (dry run) data:kafka:old_jmx_user will be set to {old_jmx_user_value}")
        else:
            updated += 1
            if row['pillar'] is None:
                print('create pillar')
                exec_command(f"{dbaas_path} --profile {profile} pillar create --sc {subcid} {{}}")

            print(f"  data:kafka:old_jmx_user will be set to {old_jmx_user_value}")
            flag = 'true' if old_jmx_user_value else 'false'
            exec_command(
                f"{dbaas_path} --profile {profile} pillar set-key --sc {subcid} -- data:kafka:old_jmx_user {flag}"
            )

    print(f'{updated} clusters were updated. {skipped} clusters were skipped due to running operation.')


def exec_command(command):
    print(command)
    result = subprocess.check_output(shlex.split(command)).decode("utf-8").strip()
    print(f'output: {result}')


def setup_metadb_connection(profile_name):
    yaml_config = load_yaml(YAML_CONFIG_PATH) or {}
    profile_config = yaml_config['profiles'].get(profile_name)
    if not profile_config:
        raise RuntimeError(f'profile {profile_name} is not supported')
    dsn = profile_config['metadb'].get('dsn')
    if not dsn:
        config = copy.deepcopy(profile_config['metadb'])
        config.pop('hosts')
        dbname = config.pop('dbname')
        port = config.pop('port', 6432)
        user = config.pop('user')
        password = config.pop('password')
        if password.startswith('sec-'):
            password = get_secret(password)['password']
        sslmode = config.pop('sslmode', 'require')
        connect_timeout = config.pop('connect_timeout', 2)
        dsn = f'port={port} dbname={dbname} user={user} password={password} sslmode={sslmode} connect_timeout={connect_timeout}'
        dsn += ''.join(f' {name}={value}' for name, value in config.items())

    hosts = ','.join(profile_config.get('metadb', {}).get('hosts', []))
    if not hosts:
        raise RuntimeError(f'profile {profile_config} is not supported. Can\'t find metadb hosts')
    jumphost_config = profile_config.get('jumphost', {})
    tunnel_params = {}
    if jumphost_config:
        port = re.match(r'port=(?P<port>\d+)', dsn).group('port')
        bind_port = get_free_port()
        tunnel_params = {
            'ssh_address_or_host': (jumphost_config['host'], 22),
            'ssh_username': jumphost_config['user'],
            'remote_bind_address': (hosts.split(',')[0], int(port)),
            'local_bind_address': ('127.0.0.1', bind_port),
        }
        hosts = 'localhost'
        dsn = re.sub(r'port=\d+', f'port={bind_port}', dsn)

    return tunnel_params, f'{dsn} host={hosts} target_session_attrs=read-write'


def exec_query(profile, query, *, fetch=True, **kwargs):
    tunnel_params, connection_string = setup_metadb_connection(profile)

    if tunnel_params:
        print(f'Start set upping ssh tunnel {tunnel_params["ssh_address_or_host"]}')
        with sshtunnel.open_tunnel(**tunnel_params):
            return exec_query_by_connection(connection_string, query, fetch=fetch, **kwargs)
    else:
        print('Start connection without ssh tunnel')
        return exec_query_by_connection(connection_string, query, fetch=fetch, **kwargs)


def exec_query_by_connection(connection_string, query, *, fetch=True, **kwargs):
    connection = psycopg2.connect(connection_string, connect_timeout=5)
    print('Connected to metadb')
    with connection.cursor(cursor_factory=DictCursor) as cursor:
        mogrified_query = cursor.mogrify(query, kwargs)
        cursor.execute(mogrified_query)
        return cursor.fetchall() if fetch else None


def get_secret(secret):
    command = f'ya vault get version --json "{secret}"'
    proc = subprocess.Popen(command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    if proc.returncode:
        raise ClickException(f'Error while execution command:\n{command}\nStderr:\n{stderr}\nStdout:\n{stdout}')

    return json.loads(stdout.decode())['value']


def get_free_port():
    with socket.socket() as s:
        s.bind(('', 0))
        return s.getsockname()[1]


if __name__ == '__main__':
    main()
