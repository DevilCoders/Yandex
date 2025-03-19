#!/usr/bin/env python3

import argparse
import enum
import pprint
import time
import subprocess
import json
import yaml
import sys
import logging

# We want different timeouts on prod and pre-prod,
# thus, we need config file
CONFIG_FILE = "/etc/yc/safe-restart/safe-restart.yaml"

DESCRIPTION = '''
This script encapsulates best practises for safe restarts of contrail services.

Purpose: deploy (called by bootstrap and salt) / recovery (called by oncall).

Steps:
 * check need-service-restart status
 * check service status via jclient-api
 * restart via specific command
 * wait for checks to be OK again
 * wait a little bit more to be safe

Notes:
 * Only after last sleep we think it's safe to restart anything else.
 * Last line of output is for salt (see "Stateful" in cmd.run docs).

Timeouts and specific commands are defined by service in config file
({}), but can be overrided by command-line options.
'''.format(CONFIG_FILE)


class JugglerStatus(int, enum.Enum):
    OK = 0
    WARN = 1
    CRIT = 2


class ExitCode(int, enum.Enum):
    OK = 0
    PRE_CHECK_FAILED = 1
    POST_CHECK_FAILED = 2


def load_config(path):
    with open(path) as f:
        return yaml.safe_load(f)


def parse_args(config, args):
    scenarios = sorted(config.keys())
    parser = argparse.ArgumentParser(description=DESCRIPTION,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-f", "--force",
                        action="store_true",
                        help="restart even if pre-checks failed (useful for recovery)")
    parser.add_argument("-t", "--wait-for-checks-timeout-sec",
                        type=int, default=None, metavar="N",
                        help="override time to wait for juggler checks to become OK (seconds)")
    parser.add_argument("-w", "--additional-delay-after-checks-sec",
                        type=int, default=None, metavar="N",
                        help="override time to wait after checks became OK after restart (seconds)")
    parser.add_argument("--checks",
                        nargs="*", default=None,
                        help="juggler checks to run before/after restart")
    parser.add_argument("scenario",
                        choices=scenarios,
                        help="restart scenario")

    args = parser.parse_args(args)
    base_config = config[args.scenario]

    return args.force, \
        args.scenario, \
        base_config["need_restart_check"], \
        base_config["restart_command"], \
        args.checks if args.checks is not None else base_config["checks"], \
        args.wait_for_checks_timeout_sec or base_config["wait_for_checks_timeout_sec"], \
        args.additional_delay_after_checks_sec or base_config["additional_delay_after_checks_sec"]


def test_parse_args():
    config = {
        "contrail-vrouter-agent-and-kernel-module": {
            "checks": ["foo", "bar"],
            "wait_for_checks_timeout_sec": 300,
            "additional_delay_after_checks_sec": 10,
            "restart_command": "restart.sh",
            "need_restart_check": "contrail-vrouter-agent"
        }
    }
    argv = ["contrail-vrouter-agent-and-kernel-module"]

    force, scenario, need_restart_check, restart_command, \
        checks, wait_for_checks_timeout_sec, additional_delay_after_checks_sec = parse_args(config, argv)

    assert not force
    assert scenario == "contrail-vrouter-agent-and-kernel-module"
    assert need_restart_check == "contrail-vrouter-agent"
    assert restart_command == "restart.sh"
    assert checks == ["foo", "bar"]
    assert wait_for_checks_timeout_sec == 300
    assert additional_delay_after_checks_sec == 10


def need_service_restart(service_name):
    result = subprocess.run(["/home/monitor/agents/modules/need-service-restart", "--check", service_name])
    return result.returncode == 0


def run_checks(checks):
    check_results = []
    for check_name in checks:
        result = subprocess.run(["jclient-api", "run-check", "--service", check_name],
                                check=True, stdout=subprocess.PIPE)
        output = json.loads(result.stdout.decode("utf-8"))
        check_results.extend(output)

    return check_results


def get_worst_status(check_results):
    if not check_results:
        return JugglerStatus.OK
    return max(JugglerStatus[result["status"]] for result in check_results)  # pylint: disable=E1136


def test_get_worst_status():
    assert get_worst_status([]) == JugglerStatus.OK
    assert get_worst_status([{"status": "OK"}]) == JugglerStatus.OK
    assert get_worst_status([{"status": "OK"}, {"status": "WARN"}]) == JugglerStatus.WARN
    assert get_worst_status([{"status": "OK"}, {"status": "CRIT"}]) == JugglerStatus.CRIT


def format_failed_checks(check_results):
    failed = [result["service"] for result in check_results if JugglerStatus[result["status"]] != JugglerStatus.OK]  # pylint: disable=E1136
    return ", ".join(failed)


def service_is_working(checks):
    logging.info("Running check: %s", checks)
    check_results = run_checks(checks)
    logging.info("Check result\n%s", pprint.pformat(check_results))
    worst_status = get_worst_status(check_results)

    ok = (worst_status == JugglerStatus.OK)
    return ok, check_results


def restart_service(scenario, restart_command):
    logging.info("restarting %r using cmd %r", scenario, restart_command)
    subprocess.run(restart_command, shell=True, check=True)
    logging.info("restarted")


def wait_for_up(checks, timeout_sec):
    logging.info("Waiting for checks %s to be OK again", checks)
    started_at = time.perf_counter()
    while True:
        time.sleep(1)

        check_results = run_checks(checks)
        worst_status = get_worst_status(check_results)
        elapsed = time.perf_counter() - started_at

        if worst_status == JugglerStatus.OK:
            break

        if elapsed > timeout_sec:
            break

    logging.info("Waiting for checks completed in %d seconds.", elapsed)
    logging.info("Last state:\n%s", pprint.pformat(check_results))

    ok = (worst_status == JugglerStatus.OK)
    return ok, check_results


def main():
    logging.basicConfig(format="%(asctime)-15s %(message)s", level=logging.INFO)
    config = load_config(CONFIG_FILE)

    force, scenario, need_restart_check, restart_command, \
        checks, wait_for_checks_timeout_sec, additional_delay_after_checks_sec = parse_args(config, sys.argv[1:])

    logging.info("force=%r, scenario=%r, need_restart_check=%r, restart_command=%r, "
                 "checks=%r, wait_for_checks_timeout_sec=%r, additional_delay_after_checks_sec=%r",
                 force, scenario, need_restart_check, restart_command,
                 checks, wait_for_checks_timeout_sec, additional_delay_after_checks_sec)

    if not force:
        logging.info("Checking that need-service-restart flag is set for %r", need_restart_check)
        if not need_service_restart(need_restart_check):
            print("changed=no comment='need-service-restart flag is not set for {}'".format(need_restart_check))
            return ExitCode.OK  # this is perfectly normal

        ok, check_results = service_is_working(checks)
        if not ok:
            failed = format_failed_checks(check_results)
            logging.info("Checks are not OK: %s. Not continuing", failed)
            print("changed=no comment='Checks are not OK: {}. Not restarting'".format(failed))
            return ExitCode.PRE_CHECK_FAILED
    else:
        logging.info("--force is used: ignoring checks before restart")

    restart_service(scenario, restart_command)
    ok, check_results = wait_for_up(checks, wait_for_checks_timeout_sec)
    if not ok:
        failed = format_failed_checks(check_results)
        logging.info("Checks not OK after restart: %s. Not continuing.", failed)
        print("changed=yes comment='Checks not OK after restart: {}'".format(failed))
        return ExitCode.POST_CHECK_FAILED

    logging.info("Sleeping after restart for %s seconds", additional_delay_after_checks_sec)
    time.sleep(additional_delay_after_checks_sec)
    print("changed=yes comment='Restarted successfully'")
    return ExitCode.OK


if __name__ == "__main__":
    sys.exit(main())
