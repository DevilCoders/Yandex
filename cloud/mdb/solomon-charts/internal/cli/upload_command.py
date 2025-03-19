from json import dumps
import logging

from click import command, pass_context, option, Choice, BadParameter

try:
    from ..lib import KIND2SOLOMON_KIND
    from ..lib.upload import upload
except ImportError:
    from importlib import import_module
    KIND2SOLOMON_KIND = import_module("cloud.mdb.solomon-charts.internal.lib").KIND2SOLOMON_KIND
    upload = import_module("cloud.mdb.solomon-charts.internal.lib.upload").upload


@command("upload")
@option('--env', required=True, help='Name of an environment')
@option('--kind', required=False, type=Choice(list(KIND2SOLOMON_KIND)), help='Update only kind entries')
@option('--oauth', required=False, help='OAuth token')
@option('--iam', required=False, help='IAM token')
@option('--pattern', required=False, help='Regexp pattern to filter entries to upload')
@pass_context
def upload_command(ctx, env, kind, oauth, iam, pattern):
    """Upload entities """
    if oauth and iam:
        raise BadParameter('there should be either oauth or iam token, not both')
    try:
        logging.debug("ctx.obj:\n%s" % dumps(ctx.obj, indent=4, sort_keys=True))
        cfg = ctx.obj['cfg']
        assert env in cfg['envs'].keys()
        if kind is not None:
            kinds = [kind]
        else:
            kinds = KIND2SOLOMON_KIND.keys()
        upload(ctx, env, kinds, oauth, iam, pattern)
    except Exception as e:
        logging.exception(e)
        exit(1)
