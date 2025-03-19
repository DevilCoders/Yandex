#!/usr/bin/env python
# -*- coding: utf-8 -*-
from .ctl_logging import log
from redis.exceptions import ConnectionError, ResponseError
from .command import CommandResult, ResultCode


def stop(conn, shutdown_cmd):
    try:
        result = conn.execute_command(shutdown_cmd)
        log.debug("shutdown_cmd result: %s", result)
    except ResponseError as err:
        log.debug("shutdown_cmd failed: %s", err)
        return CommandResult(result_print=str(err), result_code=ResultCode.ERROR)
    except ConnectionError:
        log.debug("shutdown_cmd succeeded")
        return CommandResult()
    else:
        return CommandResult(
            result_print="unexpected result - connection is not dropped: {}".format(result),
            result_code=ResultCode.ERROR,
        )
