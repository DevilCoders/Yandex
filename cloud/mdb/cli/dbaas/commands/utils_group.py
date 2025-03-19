import json
import sys

from click import argument, group, pass_context

from cloud.mdb.cli.common.parameters import JsonParamType
from cloud.mdb.cli.dbaas.internal.utils import decrypt, encrypt, generate_id


@group('utils')
def utils_group():
    """Utility commands."""
    pass


@utils_group.command('generate-id')
@pass_context
def generate_id_command(ctx):
    """Generate ID."""
    print(generate_id(ctx))


@utils_group.command('encrypt')
@argument('secret')
@pass_context
def encrypt_command(ctx, secret):
    """Encrypt secret."""
    if secret == '-':
        secret = sys.stdin.read()

    print(json.dumps(encrypt(ctx, secret)))


@utils_group.command('decrypt')
@argument('secret', type=JsonParamType())
@pass_context
def decrypt_command(ctx, secret):
    """Decrypt secret."""
    if not secret:
        if sys.stdin.isatty():
            ctx.fail('Secret to decrypt was not specified.')
        secret = sys.stdin.read()

    print(decrypt(ctx, secret))
