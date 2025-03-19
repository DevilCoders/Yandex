import os.path
from argparse import ArgumentParser
from cloud.mdb.dbaas_metadb.tests.grants.lib import get_grants_users, get_pillar_users


def main():
    parser = ArgumentParser()
    parser.add_argument('metadb_pillar_path', help='path to pill')
    parser.add_argument('new_users_dir', help='path where to store new sls')
    parser.add_argument('--grants-dir', help='path to directory with grants', default='../../../head/grants/')
    parser.add_argument('--pillar_include_prefix', default='.pgusers')

    args = parser.parse_args()
    grants_users = get_grants_users(args.grants_dir)
    pillar_users = get_pillar_users(args.metadb_pillar_path)

    for user in sorted(grants_users - pillar_users):
        with open(os.path.join(args.new_users_dir, user + '.sls'), 'w') as fd:
            fd.write(
                f"""
data:
    config:
        pgusers:
            {user}:
                superuser: False
                replication: False
                login: False
                create: True
""".lstrip()
            )
        print(f'- {args.pillar_include_prefix}.{user}')
