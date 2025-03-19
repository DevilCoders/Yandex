#!/usr/bin/env python3

import requests

import yc_monitoring

try:
    r = requests.get('http://localhost')
    r.raise_for_status()
except Exception as e:
    yc_monitoring.report_status_and_exit(yc_monitoring.Status.CRIT, str(e))
else:
    yc_monitoring.report_status_and_exit(yc_monitoring.Status.OK, 'OK')
