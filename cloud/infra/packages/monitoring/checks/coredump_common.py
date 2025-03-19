#!/usr/bin/env python3

import time
from collections import Counter
from pathlib import Path

from yc_monitoring import JugglerPassiveCheck


def check_core_dumps(check: JugglerPassiveCheck):
    core_counter = Counter()

    for path in Path('/var/crashes/').glob('core.*'):
        actual_time = time.time() - 2 * 60 * 60
        if path.is_file() and path.stat().st_mtime > actual_time:
            # stands for executable filename (%E)
            exe = path.name.split('.')[1]
            # / replaced with !
            exe = exe.split('!')[-1]
            core_counter[exe] += 1

    if len(core_counter) > 0:
        check.warn(", ".join(("{} {} cores".format(count, exe) for exe, count in core_counter.most_common())))


def main():
    check = JugglerPassiveCheck("coredump_common")
    try:
        check_core_dumps(check)
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    print(check)


if __name__ == '__main__':
    main()
