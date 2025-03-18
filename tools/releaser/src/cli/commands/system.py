# coding: utf-8

import click

from tools.releaser.src.conf import cfg


@click.command(
    help='Print configurated options and defaults'
)
def config():
    for key, value in sorted(cfg.options.items()):
        click.secho(str(key), fg='red', bold=True, nl=False)
        click.secho(': ', fg='red', bold=True, nl=False)
        click.secho(str(value), bold=True)
