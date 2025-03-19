import logging
import subprocess

from . import fail


def deleteInstance(instanceId, ycpProfile):
    cmd = ["ycp", "compute", "instance", "delete", instanceId, "--profile", ycpProfile]
    try:
        logging.debug("Running %s", cmd)
        subprocess.check_output(cmd, stderr=subprocess.PIPE)
    except subprocess.CalledProcessError as e:
        raise fail.FailException("Running {0} failed: exit code {1}:\n{2}\n{3}".format(
            cmd, e.returncode, e.output.decode("utf-8"), e.stderr.decode("utf-8")))


def rebootInstance(instanceId, ycpProfile):
    cmd = ["ycp", "compute", "instance", "restart", instanceId, "--profile", ycpProfile]
    try:
        logging.debug("Running %s", cmd)
        subprocess.check_output(cmd, stderr=subprocess.PIPE)
    except subprocess.CalledProcessError as e:
        raise fail.FailException("Running {0} failed: exit code {1}:\n{2}\n{3}".format(
            cmd, e.returncode, e.output.decode("utf-8"), e.stderr.decode("utf-8")))
