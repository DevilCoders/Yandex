"""
Utilities for dealing with gRPC services.
"""

import sys
import uuid

from cloud.mdb.cli.dbaas.internal.grpc_stateless import create_grpc_channel
from cloud.mdb.cli.dbaas.internal.iam import get_iam_token
from cloud.mdb.cli.dbaas.internal.utils import debug_mode, dry_run_mode


def grpc_request(ctx, method, request):
    if debug_mode(ctx):
        print(f'{type(request)}\n{request}', file=sys.stderr)

    if not dry_run_mode(ctx):
        metadata = (('idempotency-key', str(uuid.uuid4())),)
        return method(request, metadata=metadata)


def grpc_service(ctx, config_section, service, iam_token=None):
    return service(grpc_channel(ctx, config_section, iam_token=iam_token))


def grpc_channel(ctx, config_section, iam_token=None):
    if not iam_token:
        iam_token = get_iam_token(ctx)

    ctx_key = f'{config_section}_channel'
    if ctx_key not in ctx.obj:
        ctx.obj[ctx_key] = create_grpc_channel(ctx, config_section, iam_token)

    return ctx.obj[ctx_key]
