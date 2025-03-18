# coding: utf-8

import os
import logging
import importlib
import pkgutil
import pkg_resources
import inspect

import click

from tools.releaser.src import __version__


LOG = logging.getLogger()
logging.basicConfig(level=logging.ERROR)

os.environ['LC_ALL'] = 'en_US.UTF-8'


@click.command(cls=click.Group, help='https://a.yandex-team.ru/arc/trunk/arcadia/tools/releaser')
@click.version_option(__version__)
def cli():
    pass


def contribute_cli_commands():
    """
    # autoimport all click commands from cli.commands package
    # marked with @click.command decorator
    # and attach them to main group
    """
    import tools.releaser.src.cli.commands as commands_package

    command_modules = []

    for _, name, _ in pkgutil.iter_modules(commands_package.__path__):
        try:
            command_modules.append(
                importlib.import_module('tools.releaser.src.cli.commands.' + name)
            )
        except Exception:
            LOG.exception('Unable to attach commands from `%s` module' % name)
            continue

    for module in command_modules:
        for name, obj in inspect.getmembers(module):
            if isinstance(obj, click.Command):
                cli.add_command(obj)


contribute_cli_commands()


if __name__ == '__main__':
    cli()
