
import logging
import subprocess

log = logging.getLogger("utils")


class InternalError(Exception):
    pass


def run(*args, **kwargs):
    cmdargs = list(args)
    for optname, optval in kwargs.items():
        if optname[0] == "_":
            optname = optname.lstrip("_")

        cmdargs.append("-{}".format(optname))
        if optval is not True:
            cmdargs.append(optval)

    log.debug("Running %s", " ".join(cmdargs))
    subprocess.check_call(cmdargs)
