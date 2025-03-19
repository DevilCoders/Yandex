#!/usr/bin/env python3

import argparse
import os
import subprocess
import time


class HostGroup:
    def __init__(self, enabled, name, fqdns) -> None:
        self.enabled = enabled
        self.name = name
        self.fqdns = fqdns

    def db(self) -> bool:
        return False

    def initial_deploy_fqdn(self) -> str:
        return self.fqdns[0]


class HostGroupDB(HostGroup):
    def __init__(self, enabled, name, fqdns) -> None:
        super().__init__(enabled, name, fqdns)

    def db(self) -> bool:
        return True


def call(cmd):
    while True:
        try:
            print(f'Executing {cmd}')
            subprocess.check_call([cmd], shell=True)
            break
        except subprocess.CalledProcessError:
            time.sleep(1)


def call_remote(fqdn, cmd):
    call(f'ssh root@{fqdn} "{cmd}"')


def psql(fqdn, cmd):
    call_remote(fqdn, f'sudo -u postgres psql -d deploydb -tAX -c \\\"{cmd}\\\"')


secretsHostGroup = HostGroup(name='MDB Secrets', enabled=False,
              fqdns=[
                  'mdb-secrets01-gpn-spb99.gpn.yandexcloud.net',
                  'mdb-secrets02-gpn-spb99.gpn.yandexcloud.net'
              ])
secretsDBHostGroup = HostGroupDB(name='MDB SecretsDB', enabled=False,
              fqdns=[
                  'mdb-secretsdb01-gpn-spb99.gpn.yandexcloud.net',
                  'mdb-secretsdb02-gpn-spb99.gpn.yandexcloud.net',
                  'mdb-secretsdb03-gpn-spb99.gpn.yandexcloud.net'
              ])
saltHostGroup = HostGroup(name='MDB Salt', enabled=False,
          fqdns=[
              'mdb-salt01-gpn-spb99.gpn.yandexcloud.net',
              'mdb-salt02-gpn-spb99.gpn.yandexcloud.net',
          ])
deployHostGroup = HostGroup(name='MDB Deploy', enabled=False,
          fqdns=[
              'mdb-deploy01-gpn-spb99.gpn.yandexcloud.net',
              'mdb-deploy02-gpn-spb99.gpn.yandexcloud.net',
          ])
deployDBHostGroup = HostGroupDB(name='MDB DeployDB', enabled=False,
            fqdns=[
                'mdb-deploydb01-gpn-spb99.gpn.yandexcloud.net',
                # 'mdb-deploydb02-gpn-spb99.gpn.yandexcloud.net',
                # 'mdb-deploydb03-gpn-spb99.gpn.yandexcloud.net',
            ])

initial_deploy_hostgroups = [
    secretsHostGroup,
    secretsDBHostGroup,
    saltHostGroup,
    deployHostGroup,
    deployDBHostGroup
]

deploy_hostgroups = [
    HostGroupDB(name='MDB Internal API', enabled=True,
            fqdns=[
                'mdb-internal01-gpn-spb99.gpn.yandexcloud.net',
                'mdb-internal02-gpn-spb99.gpn.yandexcloud.net',
            ]),
]


def init_deploy_meta():
    deploydb_fqdn = deployDBHostGroup.initial_deploy_fqdn()
    master_fqdn = saltHostGroup.initial_deploy_fqdn()
    group = 'default'
    psql(deploydb_fqdn, f'SELECT * FROM code.create_group(\'{group}\') WHERE NOT EXISTS (SELECT * FROM deploy.groups WHERE name = \'{group}\')')
    psql(deploydb_fqdn, f'SELECT * FROM code.create_master(\'{master_fqdn}\', \'{group}\', true, \'\') WHERE NOT EXISTS (SELECT * FROM deploy.masters WHERE fqdn = \'{master_fqdn}\')')

    for hg in initial_deploy_hostgroups:
        minion_fqdn = hg.initial_deploy_fqdn()
        psql(deploydb_fqdn, f'SELECT * FROM code.create_minion(\'{minion_fqdn}\', \'{group}\', true) WHERE NOT EXISTS (SELECT * FROM deploy.minions WHERE fqdn = \'{minion_fqdn}\')')


def deploy_states_on_initial_deploy():
    for hg in initial_deploy_hostgroups:
        if not hg.enabled:
            continue

        fqdn = hg.initial_deploy_fqdn()
        print(f'Bootstrapping {fqdn}')

        call(f'ssh root@{fqdn} "service mdb-ping-salt-master stop"')
        call(f'ssh root@{fqdn} "echo mdb-bootstrap01k.yandexcloud.net > /etc/salt/minion_master_override"')
        call(f'ssh root@{fqdn} "service salt-minion restart"')
        subprocess.run([f'ssh root@{fqdn} "salt-call test.ping"'], shell=True)
        call(f'salt-key -y -a {fqdn}')
        call(f'salt "{fqdn}" test.ping')
        call(f'salt "{fqdn}" saltutil.sync_all')

        salt_cmd = f'salt "{fqdn}" --out=highstate --state-out=changes --output-diff --log-level=quiet state.highstate queue=True'
        if hg.db():
            salt_cmd += ' pillar=\'{"data": {"use_pgsync": False}}\''
        call(salt_cmd)


def turn_host_to_initial_deploy(fqdn):
    master_fqdn = saltHostGroup.initial_deploy_fqdn()
    call(f'ssh root@{fqdn} "rm -rf /etc/salt/minion_master_override"')
    call(f'ssh root@{fqdn} "service salt-minion restart"')
    call(f'ssh root@{fqdn} "service mdb-ping-salt-master start"')
    subprocess.run([f'ssh root@{fqdn} "salt-call test.ping"'], shell=True)
    call(f'ssh root@{master_fqdn} "salt \"{fqdn}\" test.ping"')


def turn_initial_deploy_onto_itself():
    for hg in initial_deploy_hostgroups:
        for fqdn in hg.fqdns:
            turn_host_to_initial_deploy(fqdn)


def register_minion(fqdn):
    group = 'default'
    for deploydb_fqdn in deployDBHostGroup.fqdns:
        psql(deploydb_fqdn, f'SELECT * FROM code.create_minion(\'{fqdn}\', \'{group}\', true) WHERE NOT (SELECT pg_is_in_recovery()) AND NOT EXISTS (SELECT * FROM deploy.minions WHERE fqdn = \'{fqdn}\')')


def deploy_noninitial(gpn_ca):
    for hg in deploy_hostgroups:
        for fqdn in hg.fqdns:
            register_minion(fqdn)

            call(f'scp {gpn_ca} root@{fqdn}:/opt/yandex/allCAs.pem')
            call_remote(fqdn, f'service salt-minion restart')
            call_remote(fqdn, f'salt-call test.ping')
            call_remote(fqdn, f'salt-call saltutil.sync_all')
            salt_cmd = f'salt-call --out=highstate --state-out=changes --output-diff --log-level=quiet state.highstate queue=True'
            call_remote(fqdn, salt_cmd)


def bootstrap_initial_deploy():
    deploy_states_on_initial_deploy()
    init_deploy_meta()
    turn_initial_deploy_onto_itself()


def main():
    parser = argparse.ArgumentParser(description='Bootstrap MDB deployment.')
    parser.add_argument('--initial', action='store_true', help='bootstrap initial deploy')
    parser.add_argument('--noninitial', action='store_true', help='bootstrap everything but initial deploy')
    parser.add_argument('--everything', action='store_true', help='bootstrap everything')
    parser.add_argument('--gpn_ca', default='/opt/yandex/gpnCAs.pem', help='path to GPN certificate')

    args = parser.parse_args()

    initial = args.initial or args.everything
    noninitial = args.noninitial or args.everything

    if not initial and not noninitial:
        print('Nothing to deploy')
        parser.print_help()
        return

    if initial:
        print('Bootstrapping initial deploy')
        bootstrap_initial_deploy()

    if noninitial:
        gpn_ca = args.gpn_ca
        if not os.path.isfile(gpn_ca):
            print(f'GPN CA not found at {gpn_ca}, please supply with --gpn_ca argument')
            return

        print('Bootstrapping noninitial deploy')
        deploy_noninitial(gpn_ca)


if __name__ == '__main__':
    main()
