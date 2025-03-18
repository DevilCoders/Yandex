#!/usr/bin/env bash
set -x

_scenario() {
    WALRUS = !hostcfg:h
    GLOBALseghost = !hostcfg:worm !hostcfg:h

    WALRUS A
    GLOBALseghost B: A
}

