import getpass
import logging
import os
import sys
import time
import uuid

import pytlib.raw_yt_operations as raw_yt_ops
import yaqutils.nirvana_helpers as unirv
import yaqutils.time_helpers as utime
import yt.wrapper as yt  # noqa

from pytlib.yt_retries import YT_LIGHT_RETRY_POLICY

YT_LOG_PATH = "//home/mstand/logs/operations"
YT_LOG_SCHEMA = [
    {"name": "ts", "type": "uint64", "required": True},
    {"name": "ts_msk", "type": "string", "required": True},
    {"name": "script", "type": "string", "required": True},
    {"name": "run_id", "type": "string", "required": True},
    {"name": "rectype", "type": "string", "required": True},
    {"name": "user", "type": "string", "required": True},
    {"name": "cmd", "type": "any"},
    {"name": "data", "type": "any"},
]


def get_operation_sid():
    """
    :rtype: str
    """
    return uuid.uuid4().hex


RUN_ID = get_operation_sid()


def create_log_table(yt_client):
    """
    :type yt_client: yt.YtClient
    """
    logging.info("Create log table: %s", YT_LOG_PATH)
    try:
        raw_yt_ops.yt_create_table(
            YT_LOG_PATH,
            ignore_existing=False,
            attributes={"schema": YT_LOG_SCHEMA},
            yt_client=yt_client,
            retry_policy=YT_LIGHT_RETRY_POLICY,
        )
    except:
        if not raw_yt_ops.yt_exists(YT_LOG_PATH, yt_client=yt_client):
            raise


def write_yt_log(script, rectype, yt_client, data=None):
    """
    :type script: str
    :type rectype: str
    :type yt_client: yt.YtClient
    :type data: dict[str] | None
    """
    if unirv.get_nirvana_context() and data:
        data["nirvana_data"] = _get_nirvana_data()

    debug_data = _get_debug_data()
    if debug_data and data:
        data["debug_data"] = debug_data

    ts = time.time()
    rows = [dict(
        ts=int(ts),
        ts_msk=utime.timestamp_to_iso_8601(ts),
        script=script,
        run_id=RUN_ID,
        rectype=rectype,
        cmd=sys.argv,
        user=getpass.getuser(),
        data=data,
    )]
    if not raw_yt_ops.yt_exists(YT_LOG_PATH, yt_client=yt_client):
        create_log_table(yt_client)
    raw_yt_ops.yt_append_to_table(YT_LOG_PATH, rows, yt_client=yt_client)

    logging.info(
        "Write yt log: run_id=%s, rectype=%s, operation_sid=%s",
        RUN_ID, rectype, data.get("operation_sid") if data else None,
    )


def _get_nirvana_data():
    return dict(
        block_url=unirv.get_nirvana_block_url(),
        operation_uid=unirv.get_nirvana_operation_uid(),
        process_url=unirv.get_nirvana_process_url(),
        workflow_owner=unirv.get_nirvana_workflow_owner(),
        workflow_url=unirv.get_nirvana_workflow_url(),
        workflow_uid=unirv.get_nirvana_workflow_uid(),
    )


def _get_debug_data():
    debug_keys = ("DEBUG_RUN_MODE", "DEBUG_LOWER_REDUCER_KEY", "DEBUG_UPPER_REDUCER_KEY", "DEBUG_TMP_DIR")
    return {
        key: os.environ[key]
        for key in debug_keys
        if key in os.environ
    }
