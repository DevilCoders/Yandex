#!/usr/bin/env python3


import subprocess
from cloud.netinfra.rknfilter.yc_rkn_common.custom_exceptions import reconfiguration_exception


def apply_bird_configuration_changes(logger=None):
    if logger:
        logger.debug("Applying configuration changes for Bird")
    try:
        # cmd_input = "systemctl restart bird"
        cmd_input = (
            "/usr/sbin/birdc disable FVRR_1 && /usr/sbin/birdc disable STD_B1 && systemctl restart bird"
        )
        if logger:
            logger.info("Executing: {}".format(cmd_input))
        cmd_output = subprocess.check_output(
            cmd_input, shell=True, stderr=subprocess.STDOUT
        )
        if logger:
            logger.info("Response is: {}".format(cmd_output.decode("utf-8")))
    except subprocess.CalledProcessError as exc:
        raise reconfiguration_exception(
            "Exception appeared during bird reconfiguration: {}".format(
                exc.output.decode("utf-8")
            )
        )
    if logger:
        logger.debug("Done")
