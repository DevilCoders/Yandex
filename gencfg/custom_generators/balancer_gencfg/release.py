#!/usr/bin/env python
# coding: utf-8
import click
from releaselib.cli import log, commands

logger = log.setup_logging()


@click.group()
def cli():
    """Release toolkit."""
    pass


release = commands.get_release_command(tag_url_template='https://git.qe-infra.yandex-team.ru/projects/NANNY/'
                                                        'repos/balancer-gencfg/commits?until=refs%2Ftags%2F{}')

if __name__ == '__main__':
    cli.add_command(release)
    cli.add_command(commands.upload_to_pypi)
    cli()
