#!/usr/bin/env python3

from collections import Counter
from datetime import datetime
from typing import Dict
import json
import urllib.request
import urllib.error

from yc_monitoring import JugglerPassiveCheck

MANAGEMENT_URL = "http://localhost:8081/modules/json"
MAX_EXECUTION_TIME = 60 * 1000
MAX_LAUNCH_DELAY_IN_INTERVALS = 2
MONITORING_WINDOW = 600


class Check:
    STATUS = "exec_status"
    EXEC_TIME = "exec_time"
    DELAY = "launch_delay"


class CheckStatus:
    SUCCESS = "success"
    FAIL = "fail"


def compose_check_result(summary: Dict) -> str:
    plugin_results = []
    for plugin, checks in summary.items():
        check_results = ", ".join(
            "{} failed {} time(s)".format(key.upper(), cnt) for key, cnt in Counter(checks).items())
        plugin_results.append("{}: {}".format(plugin, check_results))
    return ", ".join(plugin_results)


def extract_plugin_name(raw_name):
    name_parts = raw_name.split("-")
    if len(name_parts) > 1:
        return name_parts[0]
    return raw_name


def get_solomon_agent_state():
    plugin_state = urllib.request.urlopen(MANAGEMENT_URL).read()
    return json.loads(plugin_state.decode())


def check_plugin_status(state):
    success = state.get("Success", False)
    return CheckStatus.SUCCESS if success else CheckStatus.FAIL


def check_plugin_exec_time(state):
    exec_time = state.get("ExecTimeMilliseconds", "0")
    if int(exec_time) < MAX_EXECUTION_TIME:
        result = CheckStatus.SUCCESS
    else:
        result = CheckStatus.FAIL
    return result


def check_plugin_launch_delay(state):
    last_run = datetime.fromtimestamp(int(state["LastRunTimestamp"]))
    delay = (datetime.now() - last_run).total_seconds()
    if delay < state["IntervalSeconds"] * MAX_LAUNCH_DELAY_IN_INTERVALS:
        result = CheckStatus.SUCCESS
    else:
        result = CheckStatus.FAIL
    return result


PLUGIN_CHECKS = {
    Check.STATUS: check_plugin_status,
    Check.EXEC_TIME: check_plugin_exec_time,
    # CLOUD-11866: Simplify check
    # Check.DELAY: check_plugin_launch_delay,
}


def check_solomon_agent(check: JugglerPassiveCheck):
    agent_state = get_solomon_agent_state()

    exec_time = datetime.now()
    failed_checks = {}
    for plugin_state in agent_state:
        last_run = datetime.fromtimestamp(int(plugin_state["LastRunTimestamp"]))
        if (exec_time - last_run).total_seconds() > MONITORING_WINDOW:
            continue

        plugin_name = extract_plugin_name(plugin_state.pop("Name"))
        for check_name, check_func in PLUGIN_CHECKS.items():
            if check_func(plugin_state) == CheckStatus.FAIL:
                if plugin_name not in failed_checks:
                    failed_checks[plugin_name] = []

                failed_checks[plugin_name].append(check_name)

    if failed_checks:
        check.warn(compose_check_result(failed_checks))


def main():
    check = JugglerPassiveCheck("solomon-agent")
    try:
        check_solomon_agent(check)
    except (urllib.error.URLError, urllib.error.HTTPError):
        check.crit("Agent unavailable")
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    print(check)


if __name__ == "__main__":
    main()
