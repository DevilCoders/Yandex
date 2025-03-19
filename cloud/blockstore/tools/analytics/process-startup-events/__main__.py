#!/usr/bin/env python3

from datetime import datetime, timedelta
import sys

# TODO: standardize log line format (https://st.yandex-team.ru/NBS-1963)

# 1. parse loglines from nbs_start.log and nbs.log => (ts, event_type, event_subtype)
# 2. find shutdown event (signal 15 received)
# 3. find startup events:
#   * init: NBS server version: XXX
#   * register: Registered dynamic node with NodeBrokerAddress: XXX
#   * component initialization: XXX initialized
#   * component startup: Started XXX
#   * server listening start: Listen on XXX
#   * mount starts: Mounting volume: "$volumename" => extract volumename-s
#   * mount successes: Mount completed for client XXX , volume "$volumename", XXX - match with mount starts
# 4. output stage \t time for each stage, sort by stage length

EV_SHUTDOWN = "shutdown"
EV_START = "start"
EV_REGISTER = "register"
EV_COMPONENT_INIT = "comp_init"
EV_COMPONENT_START = "comp_start"
EV_LISTEN = "listen"
EV_MOUNT_START = "mount_start"
EV_MOUNT_END = "mount_end"
EV_LAST_VOLUME_MOUNTED = "last_mount"
# 2 fake events
EV_ALL_VOLUMES_MOUNTED = "all_mounted"
EV_TOTAL = "total"


class LogRecord(object):

    def __init__(self, ts, event_type, event_subtype):
        self.ts = ts
        self.event_type = event_type
        self.event_subtype = event_subtype

    def describe(self):
        if self.event_subtype is not None:
            return "ts=%s [type=%s subtype=%s]" % (self.ts, self.event_type, self.event_subtype)

        return "ts=%s [type=%s]" % (self.ts, self.event_type)


def log_line_to_parts(s):
    assert s.find("LOGGER ERROR") < 0

    # XXX
    assert s[5] != " "
    assert s[6] == " "
    s = s[7:]
    return s.split(" ")


def parse_nbs_start_record(s):
    parts = log_line_to_parts(s)

    ts = datetime.strptime(parts[0], "%H:%M:%S")

    if s.find("signal 15 received") >= 0:
        return LogRecord(ts, EV_SHUTDOWN, None)

    if s.find("NBS server version") >= 0:
        return LogRecord(ts, EV_START, None)

    if s.find("Registered dynamic node with") >= 0:
        return LogRecord(ts, EV_REGISTER, None)

    if len(parts) > 7:
        if parts[-1] == "initialized":
            component = " ".join(parts[7:len(parts) - 1])
            return LogRecord(ts, EV_COMPONENT_INIT, component)

        if parts[7] == "Started":
            component = " ".join(parts[8:])
            return LogRecord(ts, EV_COMPONENT_START, component)

    return None


def parse_nbs_log_record(s):
    parts = log_line_to_parts(s)

    ts = datetime.strptime(parts[0], "%H:%M:%S")

    if s.find("Listen on (data) ") >= 0:
        return LogRecord(ts, EV_LISTEN, None)

    if len(parts) > 6:
        if parts[6] == "Mounting":
            disk_id = parts[8]
            return LogRecord(ts, EV_MOUNT_START, disk_id)

        if parts[6] == "Mount" and parts[7] == "completed":
            disk_id = parts[13][:-1]
            return LogRecord(ts, EV_MOUNT_END, disk_id)

    return None


def parse_records(log, parser):
    records = []
    with open(log) as f:
        for line in f.readlines():
            rec = parser(line.rstrip())
            if rec is not None:
                records.append(rec)

    return records


if __name__ == "__main__":
    records = parse_records(sys.argv[1], parse_nbs_start_record)
    records += parse_records(sys.argv[2], parse_nbs_log_record)
    records.sort(key=lambda x: x.ts)

    last_shutdown_idx = None

    for i in range(len(records)):
        if records[-1 - i].event_type == EV_SHUTDOWN:
            last_shutdown_idx = len(records) - 1 - i
            break

    records = records[last_shutdown_idx:]

    prev_ts = None
    durations = []
    mount_starts = {}
    mount_ends = set()
    listen_ts = None
    max_mount_duration = timedelta(seconds=0)
    first_mount_ts = None
    last_mount_ts = None

    max_mount_disk_id = None

    for rec in records:
        print(rec.describe(), file=sys.stderr)

        if rec.event_type == EV_LISTEN:
            listen_ts = rec.ts

        if rec.event_type == EV_MOUNT_START:
            if listen_ts is not None and rec.ts - listen_ts <= timedelta(seconds=5):
                mount_starts[rec.event_subtype] = rec.ts
                if first_mount_ts is None:
                    first_mount_ts = rec.ts
            continue

        if rec.event_type == EV_MOUNT_END:
            mount_start = mount_starts.get(rec.event_subtype)
            mount_end_observed = rec.event_subtype in mount_ends
            if mount_start is not None and not mount_end_observed:
                if rec.ts - mount_start > max_mount_duration:
                    max_mount_disk_id = rec.event_subtype
                max_mount_duration = max(max_mount_duration, rec.ts - mount_start)
                del mount_starts[rec.event_subtype]
                mount_ends.add(rec.event_subtype)
                last_mount_ts = rec.ts

            continue

        if prev_ts is not None:
            duration = rec.ts - prev_ts
            durations.append((duration, rec))
        prev_ts = rec.ts

    print(
        "max_mount_disk_id: %s, mount_duration: %s" % (max_mount_disk_id, max_mount_duration),
        file=sys.stderr
    )

    total_mount_duration = last_mount_ts - first_mount_ts
    durations.append((max_mount_duration, LogRecord(last_mount_ts, EV_LAST_VOLUME_MOUNTED, None)))
    durations.append((total_mount_duration, LogRecord(last_mount_ts, EV_ALL_VOLUMES_MOUNTED, None)))
    durations.append((last_mount_ts - records[0].ts, LogRecord(last_mount_ts, EV_TOTAL, None)))

    durations.sort(key=lambda x: -x[0])
    for d in durations:
        print("duration=%s %s" % (d[0].seconds, d[1].describe()))
