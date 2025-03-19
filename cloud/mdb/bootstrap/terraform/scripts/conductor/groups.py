import argparse
import logging

from client import ConductorClient, get_config


def _main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--project', type=str, help='Project name')
    parser.add_argument('-f', '--parent', type=str, help='Parent for the group')
    parser.add_argument('cmd', choices=['add', 'del'], type=str, help='Operation')
    parser.add_argument('groups', metavar='fqdn', type=str, nargs='+', help='target fqdns')
    args = parser.parse_args()

    config = get_config()
    client = ConductorClient(config)

    logging.basicConfig(format=logging.BASIC_FORMAT, level=logging.DEBUG)
    logger = logging.getLogger('main')

    if args.cmd == 'add':
        logger.info('Start add host(s)')
        for i in ['parent', 'project']:
            if not getattr(args, i, None):
                print(f'{i} is required for group creation')
                parser.print_help()
                return
        for group in args.groups:
            try:
                client.ensure_group_create(group, args.project, args.parent)
                print(f'{group} created')
            except Exception as exc:
                print(f'{group} error: {repr(exc)}')
    else:
        for group in args.groups:
            try:
                client.ensure_group_delete(group)
                print(f'{group} deleted')
            except Exception as exc:
                print(f'{group} error: {repr(exc)}')


if __name__ == '__main__':
    _main()
