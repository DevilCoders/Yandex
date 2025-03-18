"""
    Script utils/heatup.py tests:
        - test if it can work in a dry run
"""

import socket
import os

import gencfg
from gaux.aux_utils import run_command
from config import MAIN_DIR


def test_heatup(curdb):
    return True  # FIXME: tempoarary disable, because sky run does not work on sandbox machines properly

    args = [os.path.join(MAIN_DIR, "utils", "sky", "heatup.py"), "-l", "msk", "-f", "+%s" % socket.gethostname(), "-q",
            "0", "--dryrun", "-vvv"]

    _code, _out, _err = run_command(args, raise_failed=False, timeout=50)

    assert _code == 0, "Command <%s> failed with status <%s>\nOutput: <%s>\nStderr: <%s>" % (args, _code, _out, _err)
