#!/usr/bin/env python

import enum
import traceback
import warnings


class Status(enum.Enum):
    OK = 0
    WARN = 1
    CRIT = 2


def die(code, msg):
    print('{};{}'.format(
        code.value,
        msg.replace('\n', ' ').replace(';', ' ')[:500]
    ))
    exit(0)


def try_or_die(some_callable):
    warnings.filterwarnings("ignore")  # to not mess with monrun output
    try:
        some_callable()
    except Exception as exc:
        die(Status.CRIT, 'python exception: "{exc}". {trace}'.format(exc=exc, trace=traceback.format_exc()))
