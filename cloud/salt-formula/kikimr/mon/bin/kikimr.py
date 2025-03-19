#!/usr/bin/env python3
import subprocess
import os.path
import yaml
import time

CONFIG_FILE = "/home/monitor/agents/etc/kikimr_tenants.conf"
VALID_UPTIME = 10


class CheckResult(object):
    STATUS_OK = 0
    STATUS_WARN = 1
    STATUS_CRIT = 2

    def __init__(self):
        self.statuses = []
        self.messages = []

    def add_result(self, status, msg):
        self.statuses.append(status)
        self.messages.append(msg)

    def ok(self, msg='OK'):
        self.add_result(self.STATUS_OK, msg)

    def warn(self, msg):
        self.add_result(self.STATUS_WARN, msg)

    def crit(self, msg):
        self.add_result(self.STATUS_CRIT, msg)


def final_message(s, m):
    print("PASSIVE-CHECK:kikimr;{};{}".format(max(s), ', '.join(m)))
    exit()

def get_tenants(config_path):
    if os.path.exists(config_path):
        with open(config_path) as config:
            return yaml.load(config)
    else:
        return []

def is_active_running(tenant=None):
    result = CheckResult()
    service_properties = {}
    service = "kikimr"
    if tenant:
        service = "kikimr@{}".format(tenant)
    cmd = "/bin/systemctl show {}.service".format(service)
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    stdout_list = proc.communicate()[0].decode('utf-8').split('\n')
    for service_property in stdout_list:
        if service_property:
            k, v = service_property.split("=", 1)
            service_properties[k] = v
    if service_properties["ActiveState"] == "active" and service_properties["SubState"] == "running":
        uptime_service = time.time() -  time.mktime(time.strptime(service_properties["ExecMainStartTimestamp"], "%a %Y-%m-%d %H:%M:%S %Z"))
        if uptime_service > VALID_UPTIME:
            result.ok()
        else:
            result.warn('Uptime {} is {} seconds'.format(service, int(uptime_service)))
    else:
        result.crit("{} is not running".format(service))
    return result

def main():
    status, message = [], []
    tenants = get_tenants(CONFIG_FILE)
    if tenants:
        for tenant in tenants:
            service_status = is_active_running(tenant=tenant)
            status.extend(service_status.statuses)
            message.extend(service_status.messages)
    else:
        service_status = is_active_running()
        status.extend(service_status.statuses)
        message.extend(service_status.messages)

    if max(status) != 0:
        message = filter(lambda x: True if x != 'OK' else False, message)
    else:
        message = list(set(message))

    final_message(status, message)

if __name__ == '__main__':
    main()
