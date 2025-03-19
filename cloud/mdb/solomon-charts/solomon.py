#!/usr/bin/env python3
import logging

from click import pass_context, group, option

try:
    from internal.cli.logs import conf_logging
    from internal.cli.render_command import render_command
    from internal.cli.upload_command import upload_command
    from internal.lib.config import load_config
except ImportError:
    from importlib import import_module
    conf_logging = import_module("cloud.mdb.solomon-charts.internal.cli.logs").conf_logging
    render_command = import_module("cloud.mdb.solomon-charts.internal.cli.render_command").render_command
    upload_command = import_module("cloud.mdb.solomon-charts.internal.cli.upload_command").upload_command
    load_config = import_module("cloud.mdb.solomon-charts.internal.lib.config").load_config


__cfg_file = 'solomon.json'


@group(context_settings={
    'help_option_names': ['-h', '--help'],
    'terminal_width': 120,
})
@option('-d', '--debug', is_flag=True, help="Enable debug output.")
@option('--dry-run', is_flag=True, help="Enable dry run mode.")
@pass_context
def cli(ctx, debug: bool, dry_run: bool):
    """ Solomon CLI tool"""
    level = logging.DEBUG if debug else logging.INFO
    conf_logging(level)

    ctx.obj = dict(debug=debug, dry_run=dry_run)
    ctx.obj['cfg'] = load_config(ctx, __cfg_file)


cli.add_command(render_command)
cli.add_command(upload_command)

if __name__ == '__main__':
    cli()
