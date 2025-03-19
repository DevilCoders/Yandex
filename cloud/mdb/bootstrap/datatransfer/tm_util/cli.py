import logging
import yaml
import os
from .api import TM
from .jinja import render_config
from .yav import yav
from . import util
import click

CONFIG = './config.yaml'


@click.group('main')
@click.option('--conf', type=click.Path(), required=True, help='path to the config file')
@click.option('--debug', type=bool, help='verbose output', is_flag=True, default=False)
@click.pass_context
def cli(ctx, conf, debug):
    logging.basicConfig(level=logging.DEBUG if debug else logging.INFO)
    config = yaml.safe_load(
        render_config(conf, context={'yav': yav, 'env': os.environ}))
    ctx.ensure_object(dict)
    ctx.obj.update({
        'api': TM(
            endpoint=config['endpoint'],
            folder_id=config['folder_id'],
            token=config['token'],
            iam_url=config.get('iam_url')),
        'config': config,
        'log': logging.getLogger('tm'),
    })


@click.command()
@click.pass_context
@click.option('-1', 'idonly', is_flag=True, default=False, help='show id only')
@click.option('--id', 'xfer_id', type=str, help='ID of transfer', default=None)
def status(ctx, idonly, xfer_id):
    util.status(ctx.obj['api'], idonly, xfer_id=xfer_id)


@click.command()
@click.pass_context
@click.option('--id', 'entity', required=True, type=str, help='ID of transfer')
def logs(ctx, entity):
    util.logs(ctx.obj['api'], entity)


@click.command()
@click.option('--dry-run', 'dry_run', type=bool, help='show diff and exit', is_flag=True, default=False)
@click.pass_context
def create(ctx, dry_run):
    util.create(ctx.obj['config'], ctx.obj['api'], dry_run=dry_run)


@click.command()
@click.pass_context
@click.option('--id', 'xfer_id', type=str, help='ID of transfer', default=None)
def start(ctx, xfer_id):
    util.start(ctx.obj['api'], xfer_id=xfer_id)


@click.command()
@click.pass_context
@click.option('-1', 'idonly', is_flag=True, default=False, help='show id only')
def endpoints(ctx, idonly):
    util.list_endpoints(ctx.obj['api'], idonly)


@click.command()
@click.pass_context
@click.option('--id', 'xfer_id', type=str, help='ID of transfer', default=None)
def deactivate(ctx, xfer_id):
    util.deactivate(ctx.obj['api'], xfer_id)


@click.command(name='drop-orphans')
@click.pass_context
@click.option('--dry-run', 'dry_run', is_flag=True, help='dont delete, just print', default=False)
def drop_orphans(ctx, dry_run):
    util.drop_orphans(ctx.obj['api'], dry_run=dry_run)


cli.add_command(status)
cli.add_command(logs)
cli.add_command(create)
cli.add_command(start)
cli.add_command(endpoints)
cli.add_command(deactivate)
cli.add_command(drop_orphans)
