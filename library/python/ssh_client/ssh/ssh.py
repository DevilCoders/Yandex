import sys
import argparse
from library.python.ssh_client import SshClient, DEVNULL


def main():
    parser = argparse.ArgumentParser(description='SSH client')
    parser.add_argument('-l', default=None, dest='username')
    parser.add_argument('-n', dest='null', action='store_true', help='redirect stdin to /dev/null')
    parser.add_argument('-p', dest='port', default=22, type=int)
    parser.add_argument('hostname', metavar='[user@]hostname')
    parser.add_argument('command', nargs='*')
    args = parser.parse_args()

    if '@' in args.hostname:
        args.username, args.hostname = args.hostname.split('@', 1)

    if not args.command:
        args.command = None

    client = SshClient(hostname=args.hostname, username=args.username, port=args.port)
    ret = client.call(args=args.command, shell=True, stdin=DEVNULL if args.null else None)
    sys.exit(ret)
