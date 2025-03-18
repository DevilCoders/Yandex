import sys
import subprocess

import click

from ylock import create_manager


@click.group()
def main():
    pass


@main.command()
@click.argument('name')
@click.argument('cmd')
@click.option('--timeout', '-t', default=5, help='lock timeout')
@click.option('--host', '-h', default=[], help='host')
@click.option('--block', '-l', default=False, help='wait lock')
@click.option('--backend', '-b', default='dispofa', help='backend name')
def lock(name, cmd, timeout,
         host, block, backend,
         ):
    manager = create_manager(backend, hosts=host)

    with manager.lock(name, timeout, block=block) as locked:
        if not locked:
            return 0

        return subprocess.call(cmd, shell=True)


if __name__ == '__main__':
    sys.exit(main())
