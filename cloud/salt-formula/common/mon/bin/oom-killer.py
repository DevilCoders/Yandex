#!/usr/bin/env python
# -*- coding: utf-8 -*-
import re
from collections import defaultdict
from datetime import datetime, timedelta
from systemd import journal

TIME_WINDOW_SIZE_MIN = 60


class Status(object):
    OK = 0
    WARNING = 1
    CRITICAL = 2


class Check(object):
    NAME = "oom-killer"
    TYPE = "PASSIVE-CHECK"


class Messages(object):
    OK = "OK"


class MessagePrinter(object):

    def __init__(self):
        self._message_severity = []
        self._message_text = []

    def add(self, message_severity, message_text):
        self._message_severity.append(message_severity)
        self._message_text.append(message_text)

    @property
    def messages_count(self):
        return len(self._message_severity)

    @property
    def out(self):
        return "{}:{};{};{}".format(
            Check.TYPE,
            Check.NAME,
            max(self._message_severity),
            "; ".join(self._message_text)
        )


class OOMStat(object):
    OOM_PATTERN = re.compile(r'^Out of memory: Kill process\ (?P<pid>\d+)\ [(](?P<process_name>\w+)[)].*')

    def read_messages(self, time_window_size):
        journal_records = journal.Reader()
        journal_records.seek_realtime(datetime.now() - timedelta(minutes=time_window_size))
        journal_records.add_match(PRIORITY='3', _TRANSPORT='kernel')

        for entry in journal_records:
            match = self.OOM_PATTERN.match(entry['MESSAGE'])
            if match:
                message = {}
                message['time'] = entry['__REALTIME_TIMESTAMP']
                message.update(match.groupdict())
                yield message


def main():

    out_message = MessagePrinter()
    oom_messages = OOMStat()

    result = defaultdict(list)
    for item in oom_messages.read_messages(TIME_WINDOW_SIZE_MIN):
        result[item['process_name']].append((item['pid'], item['time']))

    messages = []

    for process_name, ooms in result.items():
        messages.append(
            '{}: [{}];'.format(
                process_name,
                ', '.join(
                    ('{} - {}'.format(pid, time.strftime("%H:%M:%S")) for pid, time in ooms)
                )
            )
        )

    if messages:
        out_message.add(Status.CRITICAL, " ".join(messages)[:-1])
    else:
        out_message.add(Status.OK, Messages.OK)

    return out_message.out


if __name__ == '__main__':
    print(main())
    exit(0)
