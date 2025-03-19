#!/usr/bin/env pynoc

import argparse
import time
from datetime import datetime
from functools import partial
import json
import logging as log
import os
import re
from dateutil import parser as dateparser
from typing import Any, Callable, Optional, ClassVar
import requests
import dataclasses


KIND_GAUGE = "GAUGE"
KIND_RATE = "RATE"
GAUGE_NAMES: set[str] = {"maxrss", "size", "maxqsize", "openfiles"}


@dataclasses.dataclass
class Sensor:
    object: str
    origin: str
    kind: str
    name: str
    value: int

    _name_cleaner: ClassVar[Callable[[str], str]] = partial(re.compile('[*?"\'\\`]').sub, "-")

    def __post_init__(self):
        obj_name = self._name_cleaner(self.object)
        object.__setattr__(self, "object", obj_name)


@dataclasses.dataclass
class Block:
    ts: int
    metrics: list[Sensor]


class StatsParser:
    def __init__(self, args):
        self.args = args
        self.json_errors = 0

    def parse_line(self, line: str) -> Optional[Block]:
        try:
            obj = json.loads(line.strip())
            ts: datetime = dateparser.parse(obj["time"])
            metrics: list[Sensor] = []
            for m in obj["items"]:
                if not m:
                    continue
                object_name = m.pop("name")
                origin = m.pop("origin")
                sensors = self.parse_sensors(object_name, origin, m)
                for s in sensors:
                    metrics.append(s)
            return Block(int(ts.timestamp()), metrics)
        except (json.JSONDecodeError, KeyError) as e:
            log.debug("failed to parse %r: %r", line, e)
            # ignore first line self.json_errors += 1
        return None

    def parse_sensors(self, object_name, origin, sensors_raw: dict[str, Any]) -> list[Sensor]:
        sensors = []
        for key, val in sensors_raw.items():
            if not isinstance(val, int):
                continue
            kind = KIND_GAUGE if key in GAUGE_NAMES else KIND_RATE
            sensors.append(Sensor(object_name, origin, kind, key, val))
        return sensors

    def get_stats_blocks(self, buf_size=16384) -> Optional[Block]:
        with open(self.args.log) as lh:
            now = int(time.time())
            cur_pos = lh.seek(0, os.SEEK_END)
            block: Optional[Block] = None
            raw = ''

            reads = 0
            # Read log file from end
            while cur_pos and reads < 3 and not block:
                reads += 1
                if cur_pos - buf_size < 0:
                    buf_size = cur_pos

                cur_pos -= buf_size
                lh.seek(cur_pos)
                raw_new = lh.read(buf_size)
                log.debug("Read %s of %s bytes from %s", len(raw_new), buf_size, cur_pos)
                raw = str.rstrip(raw_new + raw)

                lines = raw.split("\n")
                drop_last = 0
                for idx, line in enumerate(reversed(lines)):
                    if block := self.parse_line(line):
                        log.debug("found valid json object in line %s from end", idx + 1)
                        if block.ts + 60 < now:
                            log.warn("too old metrics block")
                            block = Block(
                                ts=now,
                                metrics=[
                                    Sensor(
                                        object="rsyslog-stats.py",
                                        origin="stats.log",
                                        kind=KIND_GAUGE,
                                        name="nodata",
                                        value=1,
                                    ),
                                ],
                            )
                        if self.json_errors:
                            block.metrics.append(
                                Sensor(
                                    object="rsyslog-stats.py",
                                    origin="parser",
                                    kind=KIND_GAUGE,
                                    name="json-errors",
                                    value=self.json_errors,
                                ),
                            )
                        return block
                    self.json_errors += 1
                    # we leave the first line, it may become complete on the next iteration of reading
                    drop_last = idx
                raw = "\n".join(lines[: len(lines) + drop_last])

        return None

    def to_solomon(self, block: Block) -> None:
        out: dict[str, list] = {'metrics': []}
        for m in block.metrics:
            out['metrics'].append(
                {
                    'kind': m.kind,
                    'labels': {
                        'origin': m.origin,
                        'obj_name': m.object,
                        'metric': m.name,
                        'sensor': f"{m.origin}.{m.object}.{m.name}",
                    },
                    'value': m.value,
                }
            )
        log.debug('JSON prepared for Solomon:\n{}'.format(json.dumps(out, indent=4)))
        if not self.args.dry_run:
            requests.post('http://localhost:10050/rsyslog', json=out)


def parse_args():
    parser = argparse.ArgumentParser('Send rsyslog stats to Solomon')
    parser.add_argument(dest="log", help="Impstats log file")
    parser.add_argument("--dry-run", help="Just parse log, don`t send data to Solomon", action="store_true")
    parser.add_argument("--iterations", help="Repeat N iterations", default=4, type=int)
    parser.add_argument("--interval", help="Iterations interval seconds", default=15, type=int)
    parser.add_argument("--loglevel", "-l", help="Logging level", default="error")
    parser.add_argument(
        '-T',
        '--time-pattern',
        dest="time_pattern",
        help="Date/time pattern to parse log",
        default='%a %b %d %H:%M:%S %Y:',
    )
    return parser.parse_args()


def run(args):
    parser = StatsParser(args)
    block = parser.get_stats_blocks()
    if block:
        parser.to_solomon(block)
    else:
        log.warning("no data for monitoring")


def main():
    args = parse_args()
    log.basicConfig(
        format='%(asctime)s %(levelname)5s %(funcName)s:%(lineno)-4s %(message)s', level=args.loglevel.upper()
    )

    interval = 0
    for _ in range(args.iterations):
        time.sleep(interval)
        interval = args.interval
        run(args)


if __name__ == '__main__':
    main()
