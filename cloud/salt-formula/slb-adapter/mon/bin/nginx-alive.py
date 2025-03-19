#!/usr/bin/env python3

import pathlib
import requests
import time

import yc_monitoring


def main():
    status_url = "http://localhost:81/ping"
    temp = pathlib.Path("/tmp/monrun-nginx-dropped")
    resp = requests.get(status_url, timeout=5)
    try:
        resp.raise_for_status()
        accepts, handled, _ = resp.text.split('\n')[2].split()
        total_dropped = int(accepts) - int(handled)
    except (requests.exceptions.HTTPError, ValueError) as e:
        yc_monitoring.report_status_and_exit(yc_monitoring.Status.CRIT, str(e))
    try:
        if temp.exists():
            prev_dropped = int(temp.read_text())
            # Work around monrun -r spoiling check results.
            if time.time() - temp.stat().st_mtime > 55:
                temp.write_text(str(total_dropped))
        else:
            prev_dropped = 0
            temp.write_text(str(total_dropped))
    except (OSError, IOError, ValueError) as e:
        # File is unusable at this point, attempt removing it.
        # There is no point in catching exceptions, we're doomed anyway.
        temp.unlink()
        yc_monitoring.report_status_and_exit(yc_monitoring.Status.CRIT, str(e))
    cur_dropped = total_dropped - prev_dropped
    if cur_dropped != 0:
        msg = "{} connections dropped".format(cur_dropped)
        yc_monitoring.report_status_and_exit(yc_monitoring.Status.CRIT, msg)
    else:
        msg = "OK"
        yc_monitoring.report_status_and_exit(yc_monitoring.Status.OK, msg)


if __name__ == "__main__":
    main()
