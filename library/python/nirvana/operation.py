import subprocess
import re
import functools
import json
import yaml

from . import job_context
from . import mr_job_context
from .util import byteify

import yt.wrapper
import yt.logger


def nirvana_operation(result=None, dumper=None):
    def wrapper(func):
        @functools.wraps(func)
        def wrapped():
            ctx = job_context.context()
            inputs = ctx.get_inputs()
            outputs = ctx.get_outputs()
            params = ctx.get_parameters()
            if result:
                assert outputs.has(result), 'result %s not in outputs' % result
            rv = func(ctx, inputs, outputs, byteify(params))
            if result:
                with open(outputs.get(result), 'w') as out:
                    if dumper == 'json':
                        json.dump(rv, out)
                    elif dumper == 'yaml':
                        yaml.dump(rv, out)
                    else:
                        out.write(str(rv))
        return wrapped
    return wrapper


def nirvana_mr_operation():
    def wrapper(func):
        @functools.wraps(func)
        def wrapped():
            ctx = mr_job_context.context()
            inputs = ctx.get_inputs()
            outputs = ctx.get_outputs()
            params = ctx.get_parameters()
            mr_inputs = ctx.get_mr_inputs()
            mr_outputs = ctx.get_mr_outputs()
            func(ctx, inputs, mr_inputs, outputs, mr_outputs, byteify(params))
        return wrapped
    return wrapper


def abort_winner_transaction(yt_client=yt.wrapper):
    def find_lock_error(inner_errors):
        for error in inner_errors:
            # https://a.yandex-team.ru/arc/trunk/arcadia/yt/python/yt/common.py?rev=3419788#L78
            if error.get("code", 0) == 402:
                return error
            error = find_lock_error(error.get("inner_errors", []))
            if error:
                return error
        return None

    def wrapper(func):
        @functools.wraps(func)
        def wrapped(*args, **kwargs):
            def handle_winner_transaction(winner_transaction):
                yt.logger.LOGGER.info('Locked by %s' % winner_transaction)
                yt.logger.LOGGER.info('Aborting transation %s' % winner_transaction['id'])
                yt.logger.LOGGER.info('Yt Client %s' % yt_client)
                yt_client.abort_transaction(winner_transaction['id'])
                func(*args, **kwargs)

            try:
                func(*args, **kwargs)
            except yt.wrapper.YtOperationFailedError as e:
                lock_error = find_lock_error(e.inner_errors)
                if lock_error:
                    winner_transaction = lock_error['attributes']['winner_transaction']
                    handle_winner_transaction(winner_transaction)
                else:
                    raise e
            except subprocess.CalledProcessError as e:
                # for operations using mapreduce-yt
                winner_transaction = re.search(r'winner_transaction (\{.*\})', e.stderr)
                if winner_transaction:
                    winner_transaction = eval(winner_transaction.group(1))
                    handle_winner_transaction(winner_transaction)
                else:
                    raise e
        return wrapped
    return wrapper
