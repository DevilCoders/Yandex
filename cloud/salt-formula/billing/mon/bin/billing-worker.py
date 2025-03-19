#!/usr/lib/yc/billing/bin/python3

import subprocess


TEMPLATE = "PASSIVE-CHECK:billing-worker;{};{}"
LEVEL_OK = 0
LEVEL_CRITICAL = 2


def get_message(level, text="OK"):
    return TEMPLATE.format(level, text)


if __name__ == '__main__':
    result = subprocess.run("systemctl is-active yc-billing-worker --quiet", shell=True).returncode
    message = get_message(LEVEL_OK) if result == 0 else get_message(LEVEL_CRITICAL, text="Worker is down")
    print(message)
