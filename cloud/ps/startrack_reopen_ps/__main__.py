import logging
import os

from cloud.ps.startrack_reopen_ps.src.startrack import StartrekClient
from cloud.ps.startrack_reopen_ps.src.check import Check
from cloud.ps.startrack_reopen_ps.config import cfg

if __name__ == "__main__":
    logging.basicConfig(
        level=logging.INFO, format="%(asctime)s %(levelname)s:%(message)s"
    )
    token = os.getenv("ST_TOKEN")
    if not token:
        logging.exception("No token for startrek client found")
        exit(1)

    stc = StartrekClient(token=token, config=cfg["startrek"])

    logging.info("Fetching tickets")
    tickets = stc.get_tickets_by_filter()
    total = len(tickets)
    logging.info("Got {} tickets. Starting main loop".format(total))
    for i, ticket in enumerate(tickets):
        if ticket is None:
            continue

        logging.info("Processing {}. {} out of {}".format(ticket.get("key"), i+1, total))
        try:
            c = Check(stc, ticket)
        except Exception as e:
            logging.exception("Error while processing ticket {}: {}".format(ticket.get("key"), e))
        else:
            c.resolve()
