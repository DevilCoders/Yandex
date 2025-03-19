#!/usr/bin/env python


import requests

from monrun_output import Status, die, try_or_die


def _main():
    resp = requests.get('http://localhost:12345/monrun/v1/send_events')
    if resp.status_code == 200:
        die(Status.OK, 'OK')

    die(Status.CRIT, resp.content)


try_or_die(_main())
