#!/usr/bin/env python3

# https://wiki.yandex-team.ru/users/annkpx/naklikivanie-novyx-shardov-s3/#2.dobavlenievkonduktornujugruppu
# Install and Setup https://github.yandex-team.ru/devtools/python-conductor-client

from conductor_client import Group, Project, Host, Datacenter

START = 33
STOP = 65

DC = {
    'i': Datacenter(name='man'),
    'k': Datacenter(name='vla'),
    'h': Datacenter(name='sas')
}

project = Project(name='mdb')
parent = Group(name='mdb_s3db_porto_prod')
parent_id = parent.id

print('Parent Group ID:', parent_id)


for n in range(START, STOP):
    gname = f'mdb_s3db{n}_porto_prod'
    group = Group(
        name=gname,
        project=project
    )
    suc = group.save()
    if not suc:
        print(group.errors())
        exit(1)
    group.parent_groups.append(parent)
    print('Created group:', gname)

    for c in ('i k h'.split()):
        hostname = f's3db{n}{c}.db.yandex.net'
        host = Host(
            fqdn=hostname,
            short_name=f's3db{n}{c}.db',
            group_id=group.id,
            datacenter=DC[c]
        )
        suc = host.save()
        if not suc:
            print(host.errors())
            exit(1)

        print('Created Host:', hostname)
