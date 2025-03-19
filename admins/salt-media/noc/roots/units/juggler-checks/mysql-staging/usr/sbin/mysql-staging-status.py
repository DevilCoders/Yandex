#!/usr/bin/env python3

import os
import time
import sys
import random
import tempfile
import argparse
import subprocess
import logging as log
from os import path
from typing import List, Dict, Any, Optional

import platform
import yaml

log.basicConfig(
    format="%(asctime)s %(levelname)5s %(funcName)s:%(lineno)-4s %(message)s",
    level=log.CRITICAL,
    stream=sys.stderr,
)


OK = "OK"
WARN = "WARN"
CRIT = "CRIT"

MAX_REPL_LAG = 600  # seconds

STATUS = "PASSIVE-CHECK:mysql-status;{};{}"
MYSYNC_CONF = "/etc/mysync.yaml"
STATUS_HISTORY = "/dev/shm/mysql-staging-status-history"
STATUS_RANNING = "running"
STATUS_HISTORY_PERIOD = 300


def report(level: str, msg: str) -> None:
    print(STATUS.format(level, msg.strip()))
    sys.exit(0)


def get_status_err(status: Dict[str, Any]) -> str:
    err = status.get("error", "")
    return f"err {err}" if err else ""


def get_status_from_zk(node: str) -> Dict[str, Any]:
    with open(MYSYNC_CONF) as mc:
        conf = yaml.safe_load(mc)
    namespace: str = conf["zookeeper"]["namespace"]
    zk_servers: List[str] = conf["zookeeper"]["hosts"]
    log.debug("lookup zk data in namespace: %r from servers: %r", namespace, zk_servers)
    random.shuffle(zk_servers)
    conn_str = ",".join(zk_servers)

    cmd = ["zk", "-s", conn_str, "get", path.join(namespace, "health", node)]
    log.debug("run cmd: %r", cmd)
    err = None
    try:
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, errBytes = p.communicate()
        err = errBytes.decode("utf8")
        if p.returncode == 0:
            return yaml.safe_load(out)
    except Exception as e:
        err = f"{e!r}"
    if not err:
        err = "unexpected behaviour..."
    return {"error": f"Failed to get status from zk: {err!r}"}


def save_status_history(status: str) -> bool:
    now = time.time()
    with open(STATUS_HISTORY, "a"):
        pass

    with open(STATUS_HISTORY) as sh:
        hist = yaml.safe_load(sh)
    if hist is None:
        hist = []
    last_statuses = list(h for h in hist if now - h["ts"] < 300)
    last_statuses.append({"ts": now, "status": status})
    tmpf = tempfile.NamedTemporaryFile(
        "w", dir=path.dirname(STATUS_HISTORY), delete=False
    )
    log.debug("Write truncated repl status history to %r: %s", tmpf.name, last_statuses)
    tmpf.write(yaml.dump(last_statuses))
    tmpf.close()
    os.rename(tmpf.name, STATUS_HISTORY)
    # is stable down
    return not any(s["status"] == STATUS_RANNING for s in last_statuses)


def check_freshness(date: str) -> bool:
    if not date:
        return False
    ts = time.mktime(time.strptime(date[:19], "%Y-%m-%dT%H:%M:%S"))
    return time.time() - ts > MAX_REPL_LAG


def main(test_data: str):
    node = platform.node()
    if test_data:
        log.debug("load data from mock file %r", test_data)
        with open(test_data) as td:
            status = yaml.safe_load(td)
    else:
        log.debug("load status data from zk")
        status = get_status_from_zk(node)

    check_date = status.get("check_at", "0001-01-31T13:26:53.276486147+03:00")
    log.debug("Status check date %s", check_date)
    if check_freshness(check_date):
        err = get_status_err(status)
        if "zk: node does not exist" in err:
            import docker

            cli = docker.DockerClient()
            c = cli.containers.get("rt-mysql-main")
            if not any("rt-mysql" in tag for tag in c.attrs["Config"]["Image"]):
                report(WARN, "Old rt-mysql-main container?")
        report(CRIT, f"Status too old: {check_date} {get_status_err(status)}")

    log.debug(f"status: {yaml.dump(status)}")
    if not status.get("is_cascade", False):
        report(CRIT, f"mysql node is not cascade node {get_status_err(status)}")

    slave_state: Dict[str, Any] = status.get("slave_state", {})
    if not slave_state:
        report(CRIT, f"Failed to get slave_state {get_status_err(status)}")

    repl_state: str = slave_state.get("replication_state", "unknown")
    is_stable_down = save_status_history(repl_state)
    if repl_state != STATUS_RANNING and is_stable_down:
        report(CRIT, f"Repl state is: {repl_state} {get_status_err(status)}")

    repl_lag: Optional[int] = slave_state.get("replication_lag")
    if repl_lag is not None and repl_lag > MAX_REPL_LAG:
        report(CRIT, f"Repl lag is too large: {repl_lag} {get_status_err(status)}")
    report(OK, "OK")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-d", "--debug", action="store_true", help="Debug")
    parser.add_argument("--test-data", help="mock zk status for testing")
    args = parser.parse_args()
    if args.debug:
        log.getLogger().setLevel(log.DEBUG)

    main(args.test_data)
