#!/usr/bin/env python3

import sys


def percentile(l, p):
    assert len(l) > 0

    index = int(len(l) * p)
    if index == len(l):
        index = len(l) - 1

    return l[index]


if __name__ == '__main__':
    stage2durations = {}

    with open(sys.argv[1]) as f:
        for line in f.readlines():
            line = line.rstrip()
            if not line.startswith("duration="):
                continue

            parts = line.split(" ", 3)
            d = int(parts[0][9:])
            stage = parts[3]

            if stage not in stage2durations:
                stage2durations[stage] = []

            stage2durations[stage].append(d)

    stages = []

    for stage, durations in stage2durations.items():
        durations.sort()
        stages.append((percentile(durations, 0.99), stage, durations))

    stages.sort(key=lambda x: -x[0])

    for _, stage, durations in stages:
        print()
        print("stage=%s" % stage)
        for p in [1, 0.99, 0.9, 0.75, 0.5, 0.25, 0.1, 0.01, 0]:
            print("p%s=%s" % (p, percentile(durations, p)))

