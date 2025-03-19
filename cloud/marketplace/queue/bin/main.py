#!/usr/bin/env python3
import os
from time import sleep

from yc_common import logging

from cloud.marketplace.queue.yc_marketplace_queue.config import load
from cloud.marketplace.queue.yc_marketplace_queue.main import main as q


def main():
    devel = os.getenv("DEVEL")
    loglevel = os.getenv("LOGLEVEL")
    logging.setup(
        devel_mode=(devel is not None),
        debug_mode=True,
        level=loglevel,
        context_fields=[("request_id", "r"), ("request_uid", "u"), ("operation_id", "o")]
    )
    load(devel)

    while True:
        q()
        sleep(1)


if __name__ == "__main__":
    main()
