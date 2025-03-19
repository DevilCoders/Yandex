{% from "components/postgres/pg.jinja" import pg with context %}
#!/usr/bin/env python

import argparse
import sys
import socket
import os

parser = argparse.ArgumentParser(
        description='Script for generating recovery.conf on replics',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
        )


parser.add_argument('-p', '--path',
                    type=str,
{% if pg.version.major_num < 1200 %}
                    default='{{ pg_prefix }}/recovery.conf',
{% else %}
                    default='{{ pg_prefix }}/conf.d/recovery.conf',
{% endif %}
                    help='Full path to recovery.conf')

parser.add_argument('master',
                    type=str,
                    metavar='MASTER',
                    help='Replication source or master hostname')

parser.add_argument('-s', '--no-replication-slot',
                    action='store_true',
                    default=False,
                    help='Disable replication slot')

def_slot_name = socket.gethostname().replace('.', '_').replace('-', '_')
parser.add_argument('-n', '--slot-name',
                    type=str,
                    default=def_slot_name,
                    help='Name of the replication slot to use')

parser.add_argument('-d', '--apply-delay',
                    type=str,
                    metavar='<int>[min|sec]',
                    default=0,
                    help='Apply artificial delay in WAL processing')

parser.add_argument('-r', '--empty-arg',
                    action='store_true',
                    default=False,
                    help='The script behavior changes and it applies restore-command by default!!!')
parser.add_argument('-e', '--exclude-restore-command',
                    action='store_true',
                    default=False,
                    help='Do not put restore_command into the recovery.conf.')
{% if salt['pillar.get']('data:use_walg', True) %}
def_cmd = '/usr/bin/timeout -s SIGQUIT 60 /usr/bin/wal-g wal-fetch "%f" "%p" --config /etc/wal-g/wal-g.yaml'
{% else %}
def_cmd = '/bin/false'
{% endif %}
parser.add_argument('-c', '--restore-command',
                    type=str,
                    default=def_cmd,
                    help='Restore command')

def_conninfo = 'port=5432 user=repl application_name=%s{% if salt['pillar.get']('data:pg_ssl', True) %} sslmode=verify-full{% endif %}' % def_slot_name
parser.add_argument('-o', '--connection-options',
                    type=str,
                    default=def_conninfo,
                    help='Extra options for primary_conninfo')

args = parser.parse_args()

try:
    f = open(args.path, 'w')
{% if pg.version.major_num < 1200 %}
    f.write("standby_mode = 'on'\n")
{% else %}
    with open('{{ pg_prefix }}/standby.signal', 'w') as ssf:
        ssf.write('')
{% endif %}
    f.write("recovery_target_timeline = 'latest'\n")
    f.write("primary_conninfo = 'host=%s %s'\n" % (args.master,  args.connection_options))
    if not args.no_replication_slot:
        f.write("primary_slot_name = '%s'\n" % args.slot_name)
    if not args.exclude_restore_command:
        f.write("restore_command = '%s'\n" % args.restore_command)
    if args.apply_delay:
        f.write("recovery_min_apply_delay = '%s'\n" % args.apply_delay)
    f.close()

    dir = os.stat(os.path.dirname(args.path))
    if dir.st_uid == os.geteuid():
        sys.exit(0)
    if os.geteuid() == 0:
        # We are root so it is better to change ownership on created file
        os.chown(args.path, dir.st_uid, dir.st_gid)
    else:
        print('Not trying to change ownership on created file.')
    sys.exit(0)
except Exception as err:
    print(err)
    f.close()
    sys.exit(1)
