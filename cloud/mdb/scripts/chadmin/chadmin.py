#!/usr/bin/env python3
import argparse
import sys
from os.path import basename

from lib.utils import execute, upload_to_paste
from lib.diagnostics import collect_diagnostics, format_diagnostics_data

USAGE = '''[<option>] ... <host> <command>
commands:
  diagnostics          Collect diagnostics data and upload it to Ya Paste.   
  collect-diagnostics  Collect diagnostics data and save it to a file in the current directory.
  <chadmin_command>    Execute arbitrary administration command on remote host. 
  help                 Show help with available administration commands on remote host.
options:
  --pssh               Use pssh to execute commands on remote host (dafault behavior).
  --ssh                Use ssh instead of pssh to execute commands on remote host.
  --sudo               Execute commands on remote host with sudo (dafault behavior).
  --no-sudo            Execute commands on remote host without sudo.
'''

def main():
    usage = f'{basename(sys.argv[0])} {USAGE}'
    parser = argparse.ArgumentParser(add_help=False, usage=usage)
    parser.add_argument('--sudo', action='store_true', dest='sudo', default=True)
    parser.add_argument('--no-sudo', action='store_false', dest='sudo')
    parser.add_argument('--pssh', action='store_true', dest='use_pssh', default=True)
    parser.add_argument('--ssh', action='store_false', dest='use_pssh')
    parser.add_argument('host')
    parser.add_argument('command')
    args, unparsed = parser.parse_known_args()

    if args.command == 'collect-diagnostics':
        collect_diagnostics_command(args.host, *unparsed, use_pssh=args.use_pssh, sudo=args.sudo)
    elif args.command == 'diagnostics':
        diagnostics_command(args.host, *unparsed, use_pssh=args.use_pssh, sudo=args.sudo)
    elif args.command == 'help':
        execute_chadmin(args.host, use_pssh=args.use_pssh, sudo=args.sudo)
    else:
        execute_chadmin(args.host, args.command, *unparsed, use_pssh=args.use_pssh, sudo=args.sudo)


def collect_diagnostics_command(host, *args, use_pssh, sudo):
    data = collect_diagnostics(host, *args, format='wiki', use_pssh=use_pssh, sudo=sudo)
    filename = f'{host}-diagnostics.txt'
    with open(filename, 'w') as f:
        f.write(data)
    print(f'Diagnostics data saved to {filename}')


def diagnostics_command(host, *args, use_pssh, sudo):
    data = collect_diagnostics(host, *args, format='yaml', use_pssh=use_pssh, sudo=sudo)
    formatted_data = format_diagnostics_data(host, data)
    print(upload_to_paste(formatted_data))


def execute_chadmin(host, *args, use_pssh, sudo):
    command = 'chadmin'
    if sudo:
        command = f'sudo {command}'
    command = ' '.join((command, *args))

    print(execute(host=host, command=command, use_pssh=use_pssh))


if __name__ == '__main__':
    main()
