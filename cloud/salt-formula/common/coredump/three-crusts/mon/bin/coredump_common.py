#!/usr/bin/env python3

import time
from collections import Counter
from pathlib import Path

import yc_monitoring

core_counter = Counter()

for path in Path('/var/crashes/').glob('core.*'):
    actual_time = time.time() - 2 * 60 * 60
    if path.is_file() and path.stat().st_mtime > actual_time:
        # stands for executable filename (%E)
        exe = path.name.split('.')[1]
        # / replaced with !
        exe = exe.split('!')[-1]
        core_counter[exe] += 1

if len(core_counter) == 0:
    description = "OK"
    yc_monitoring.report_status_and_exit(yc_monitoring.Status.OK, description)
else:
    parts = ["{} {} cores".format(count, exe) for exe, count in core_counter.most_common()]
    description = ", ".join(parts)
    yc_monitoring.report_status_and_exit(yc_monitoring.Status.WARN, description)
