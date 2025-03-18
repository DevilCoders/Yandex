import logging
import re

import yaqutils.requests_helpers as urequests
from yaqab.ab_client import AbClient  # noqa

TICKET_RE = re.compile("^[a-zA-Z]+-[0-9]+$")


def get_info(client, task_id):
    """
    :type client: AbClient
    :type task_id: str
    :rtype: dict[str]
    """
    if not TICKET_RE.match(task_id):
        logging.info("skip bad ticket %s", task_id)
        return None

    try:
        return client.get_task(task_id)
    except urequests.RequestPageNotFoundError:
        logging.info("there is no info for ticket %s", task_id)
        return None
