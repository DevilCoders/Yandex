import logging
from os.path import isdir
from typing import List

from click import command, pass_context, option, Choice
from json import dumps

try:
    from ..lib import KIND2SOLOMON_KIND
    from ..lib.render import render
except ImportError:
    from importlib import import_module
    KIND2SOLOMON_KIND = import_module("cloud.mdb.solomon-charts.internal.lib").KIND2SOLOMON_KIND
    render = import_module("cloud.mdb.solomon-charts.internal.lib.render").render


@command('render')
@option('-e', required=True, help="Environment")
@option('-s', required=True, help='Source directory with Jinja templates')
@option('-t', required=True, help='Target directory')
@option('-a', required=False, default=None, help="Areas: name of service, or feature")
@option('-k', required=False, multiple=True, type=Choice(list(KIND2SOLOMON_KIND)), help="Kinds")
@pass_context
def render_command(ctx, e: str, a: str, k: List[str], s: str, t: str):
    """ Render templates """
    try:
        logging.debug("ctx.obj:\n%s" % dumps(ctx.obj, indent=4, sort_keys=True))
        cfg = ctx.obj['cfg']
        assert e in cfg['envs'].keys()
        assert isdir(s)
        render(ctx, s, t, e, cfg, k, a)
    except Exception as e:
        logging.exception(e)
        exit(1)
