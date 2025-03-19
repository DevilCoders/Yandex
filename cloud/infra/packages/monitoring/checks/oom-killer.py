#!/usr/bin/env python3
"""Juggler check that triggers if oom_killer started"""

import re
from collections import defaultdict
from datetime import datetime, timedelta
from systemd import journal
from typing import Generator
from yc_monitoring import JugglerPassiveCheck

TIME_WINDOW_SIZE_MIN = 60
MSK_TIME_SHIFT = timedelta(hours=+3)
IGNORED_PROCS = ["contrail-vroute", "yc-log-reader"]


class OOMStat(object):
    OOM_PATTERN = re.compile(r'.*[Oo]ut of memory: Kill process (?P<pid>\d+) [(](?P<process_name>.+)[)].*')

    def read_messages(self, time_window_size: int) -> Generator[dict, None, None]:
        journal_records = journal.Reader()
        journal_records.seek_realtime(
            datetime.now() - timedelta(minutes=time_window_size))
        journal_records.add_match(PRIORITY='3', _TRANSPORT='kernel')

        for entry in journal_records:
            match = self.OOM_PATTERN.match(entry['MESSAGE'])
            if match:
                message = {'time': entry['__REALTIME_TIMESTAMP']}
                message.update(match.groupdict())
                yield message


def check_oom_killer(check: JugglerPassiveCheck):
    oom_messages = OOMStat()

    result = defaultdict(list)
    for item in oom_messages.read_messages(TIME_WINDOW_SIZE_MIN):
        if item['process_name'] not in IGNORED_PROCS:
            result[item['process_name']].append((item['pid'], item['time']))

    messages = []
    for process_name, ooms in result.items():
        oom_list = ("pid: {} at: {}".format(pid, (time + MSK_TIME_SHIFT).strftime("%H:%M:%S")) for pid, time in ooms)
        messages.append("process {}: [{}];".format(process_name, ", ".join(oom_list)))

    if messages:
        check.crit(" ".join(messages)[:-1])


def main():
    check = JugglerPassiveCheck("oom-killer")
    try:
        check_oom_killer(check)
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    print(check)


if __name__ == '__main__':
    main()
