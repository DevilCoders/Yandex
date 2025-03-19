"""
Helpers and wrappers module
"""

import logging

from mdb_mongo_tools.exceptions import (PredictorOperationNotAllowed, ReplSetInfoGatheringError,
                                        StepdownPrimaryWaitTimeoutExceeded, StepdownSamePrimaryElected)
from mdb_mongo_tools.mongodb import ReplSetCtl
from mdb_mongo_tools.reviewers.replset import ReplSetOpReviewer
from mdb_mongo_tools.util import retry

DEFAULT_PRIMARY_WAIT_TIMEOUT = 20


def host_shutdown_allowed(rs_op_reviewer: ReplSetOpReviewer, host_port: str) -> bool:
    """
    Host shutdown check Wrapper
    """
    try:
        rs_op_reviewer.shutdown_host(host_port)
        return True
    except PredictorOperationNotAllowed:
        return False


def stepdown(host_port: str,
             replset_ctl: ReplSetCtl,
             server_election_timeout: int,
             step_down_secs: int,
             secondary_catch_up_period_secs: int,
             ensure_new_primary: bool = True):
    """
    Stepdown wrapper
    """
    replset_info = replset_ctl.get_info()
    old_primary = replset_info.primary_host_port

    logging.info('Stepping down host %s', host_port)
    replset_ctl.stepdown_host(
        host_port=host_port,
        step_down_secs=step_down_secs,
        secondary_catch_up_period_secs=secondary_catch_up_period_secs)

    if not ensure_new_primary:
        return

    #  https://docs.mongodb.com/manual/reference/method/rs.stepDown/#behavior
    primary_election_timeout = step_down_secs + \
        server_election_timeout + \
        replset_info.config_settings.heartbeat_timeout_secs
    wait_until_primary_appears(replset_info, timeout=primary_election_timeout)

    if old_primary == replset_info.primary_host_port:
        raise StepdownSamePrimaryElected('Primary has returned to original host: {host}'.format(host=old_primary))


def wait_until_primary_appears(replset_info, timeout=None):
    """
    Wait until primary present in replset with timeout
    """
    if timeout is None:
        timeout = DEFAULT_PRIMARY_WAIT_TIMEOUT

    logging.debug('Waiting for primary until %s seconds passed', timeout)
    retry_until_primary_elected = retry(exception_types=(ReplSetInfoGatheringError, ), max_wait=timeout)
    try:
        retry_until_primary_elected(replset_info.update)()
    except ReplSetInfoGatheringError:
        raise StepdownPrimaryWaitTimeoutExceeded
