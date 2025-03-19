#!/usr/bin/env python

import os
import sys
import subprocess
import json

def send_output_message(status, message):
    if max(status) != 0:
        status = max(status)
        message = filter(lambda x: True if x != 'OK' else False, message)
    else:
        status = status[0]
        message = message[0]
    print("PASSIVE-CHECK:push-client;{};{}".format(status, message))
    sys.exit()

def get_configs(config_dir):
    if not os.path.isdir(config_dir):
        return []
    return [config_dir + x for x in os.listdir(config_dir) if x.endswith('.yaml')]

def get_all_statuses(configs_list, additional_param=None):
    raw_statuses = []
    for config in configs_list:
        command = ["/usr/bin/push-client", "-c", config, "--status", "--json"]
        if additional_param:
            command.extend(additional_param.split())
        process = subprocess.Popen(command, stdout=subprocess.PIPE)
        data = json.loads(process.communicate()[0])
        raw_statuses.extend(data)
    return raw_statuses

def check_statuses(raw_statuses):
    log_status = []
    log_message = []
    for log_info in raw_statuses:
        if log_info.get("status") != 1:
            log_status.append(2)
            log_message.append("Log status: {}. Found problem with {}".format(log_info.get("status"), log_info.get("log type")))
        if log_info.get("errors"):
            log_status.append(2)
            log_message.append("Log status: {}. Found errors during send {} logs: {}".format(log_info.get("status"), log_info.get("log type"), log_info.get("errors")))
    return log_status, log_message


if __name__ == "__main__":

    STATUS = [0]
    MESSAGE = ["OK"]
    COMMIT_LAG = 120
    SEND_LAG = 120
    CONFIG_PATH = "/etc/yandex/statbox-push-client/conf.d/"
    INTERNAL_CHECKS = "--check:send-time={} --check:commit-time={}".format(SEND_LAG, COMMIT_LAG)

    CONFIGS = get_configs(CONFIG_PATH)
    if not CONFIGS:
        STATUS.append(2)
        MESSAGE.append("Not found any push-client configs")
    ALL_RAW_STATUSES = get_all_statuses(CONFIGS, INTERNAL_CHECKS)
    LOG_STATUSES = check_statuses(ALL_RAW_STATUSES)
    STATUS.extend(LOG_STATUSES[0])
    MESSAGE.extend(LOG_STATUSES[1])
    send_output_message(STATUS, MESSAGE)
