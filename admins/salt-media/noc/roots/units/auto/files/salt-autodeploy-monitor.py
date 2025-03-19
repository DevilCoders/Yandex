#!/usr/bin/env python3
import time
import json
import platform
import logging as log
import copy
import os
import sys
import tempfile
import subprocess
from collections import Counter

OK = "OK"
WARN = "WARN"
CRIT = "CRIT"

STATE_APPLY_RESULT = "/var/tmp/salt-autodeploy-state-apply-result.json"
PILLAR_FILE = "/var/tmp/salt-autodeploy.json"

log.basicConfig(
    format="%(asctime)s %(levelname)5s %(funcName)s:%(lineno)-4s %(message)s",
    level=log.INFO,
    stream=sys.stderr,
)


def parse_state_result_from_stdin():
    try:
        state_apply_result = json.load(sys.stdin)["local"]
    except Exception as e:
        log.error("failed to load state apply result: %s", e)
        return
    if isinstance(state_apply_result, list):
        log.error("failed to apply salt states: %s", state_apply_result)
        return

    payload = json.dumps(state_apply_result).encode()
    temp_file_name = None
    with tempfile.NamedTemporaryFile(delete=False) as tf:
        temp_file_name = tf.name
        tf.write(payload)
    os.rename(temp_file_name, STATE_APPLY_RESULT)


def monitor(sls_files):
    if not os.path.exists(PILLAR_FILE):
        return CRIT, "sentinel not found"
    if not os.path.exists(STATE_APPLY_RESULT):
        return CRIT, "state apply result not found"

    state_apply_mtime = os.stat(STATE_APPLY_RESULT).st_mtime
    log.debug("State apply time: %s", state_apply_mtime)

    now = time.time()
    if now - state_apply_mtime > 3600:
        return CRIT, "salt state apply was a very long time ago"

    with open(PILLAR_FILE) as pillar_file:
        pillar = json.load(pillar_file)

    sentinel_marker = pillar["sentinel"]
    log.debug("Sentinel marser: %s", sentinel_marker)

    with open(STATE_APPLY_RESULT) as sar:
        state_apply_result = json.load(sar)

    status, description = check_state_apply_result(state_apply_result, sls_files)
    log.debug("State apply result: %s=%s", status, description)
    if status == OK:
        status, description = check_pillar_data(pillar["packages"])
        log.debug("Check pillar data status: %s=%s", status, description)

    if status == OK:
        status, description = check_pkg_is_installed(pillar["packages"])
        log.debug("Check pkg is installed status: %s=%s", status, description)

    return status, "{} {}".format(sentinel_marker, description)


def check_pkg_is_installed(data):
    versions = {}
    for pkg in data:
        versions[pkg["name"]] = pkg["version"]
    log.debug("Packages data by salt: %s", versions)

    out = subprocess.check_output(
        ["dpkg-query", "-W", "--showformat", "${Package} ${Version} ${Status}\n"]
    )
    packages = {}
    for line in out.decode().splitlines():
        line = line.strip()
        if not line:
            continue
        pkg_tuple = line.split(None, 2)
        if len(pkg_tuple) != 3:
            continue
        packages[pkg_tuple[0]] = (pkg_tuple[1], pkg_tuple[2])
    log.debug("Packages count by dpkg-query: %s", len(packages))

    ok_pkgs = []
    for name, desired_version in versions.items():
        log.debug("check pkg name=%r desired_version=%r", name, desired_version)
        installed_version, status = packages.get(name, ("", "not found"))
        log.debug(
            "dpkg result: name=%r installed_version=%r, status=%r",
            name,
            installed_version,
            status,
        )
        if status == "install ok installed" and installed_version == desired_version:
            ok_pkgs.append(name)

    log.debug("OK packages: %s", ok_pkgs)
    if len(ok_pkgs) != len(versions):
        return CRIT, "broken pkgs: {0!r}".format(
            ",".join(set(versions.keys()) - set(ok_pkgs))
        )
    return OK, "OK"


def check_pillar_data(data):
    if not data:
        return CRIT, "empty pillar data for {}".format(
            platform.node().split(".").pop(0)
        )
    count = Counter([pkg["name"] for pkg in data])
    if count.most_common(1)[0][1] > 1:
        name, cnt = count.most_common(1)[0]
        return CRIT, "duplication in pillar: {} present {} times".format(name, cnt)
    return OK, "OK"


def check_state_apply_result(payload, sls_files):
    status = OK
    description = "OK"
    checked = 0
    for state_dn, state in payload.items():
        state_file = state.get("__sls__")
        if not state_file:
            log.error("Strange state without '__sls__': %s: %s", state_dn, state)
            continue

        if sls_files and state_file not in sls_files:
            log.debug("Skip check for sls: %s", state_file)
            continue

        checked += 1
        log.debug("Check state %s for sls %s", state_dn, state_file)

        if not state.get("result"):
            log.error("failed state: %s", state_dn)
            status = WARN
            description = "salt states is broken: {}".format(
                state.get("__id__", state_dn)
            )
            break
    if not checked:
        status = WARN
        description = "All states are filtered"
    return status, description


if __name__ == "__main__":
    args = copy.copy(sys.argv[1:])
    if "--debug" in args:
        log.getLogger().setLevel(log.DEBUG)
        args.remove("--debug")

    if "--parse-stdin" in args:
        log.debug("parse stdin data")
        parse_state_result_from_stdin()
    elif "--monitor" in args[:1]:
        sls_files = args[1:]
        log.debug("run monitoring for sls files: %s", sls_files)
        status, desc = monitor(sls_files)
        print("PASSIVE-CHECK:salt-autodeploy;{};{}".format(status, desc))
    else:
        log.error("bad args %s", sys.argv)
        print(
            "usage {} [--parse-stdin|--monitor [sls-file ...]]".format(sys.argv[0]),
            file=sys.stderr,
        )
        sys.exit(1)
