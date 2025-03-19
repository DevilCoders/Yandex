import logging

import fastapi_utils.tasks
from prometheus_client import Gauge

from api.dependencies import db
from api.routes.agents import get_sync_diff

AGENTS_CONFIGURATION_UPLOADING_DIFF = Gauge(
    "agents_configuration_uploading_diff",
    "Number of bytes in diff between internal database state and agents configuration in S3",
    unit="bytes", labelnames=("filename", )
)


@fastapi_utils.tasks.repeat_every(seconds=60 * 5, raise_exceptions=True, logger=logging.getLogger(__name__))
def get_agent_configuration_current_diff():
    logging.info("Starting repeated task for calculating diff "
                 "between internal database state and agents configuration in S3")
    db_connection = next(db())
    diff = get_sync_diff(db_connection)

    AGENTS_CONFIGURATION_UPLOADING_DIFF.clear()
    total = 0
    for filename, file_diff in diff.changed_files.items():
        total += len(file_diff.new_content)
        AGENTS_CONFIGURATION_UPLOADING_DIFF.labels(filename=filename).set(len(file_diff.new_content))

    for filename in diff.unchanged_files:
        AGENTS_CONFIGURATION_UPLOADING_DIFF.labels(filename=filename).set(0)

    AGENTS_CONFIGURATION_UPLOADING_DIFF.labels(filename="-").set(total)
    logging.info(f"Total diff size is {total} bytes")

