#!/usr/bin/env python

from subprocess import check_output as _check_output

def default_iface():
    ip_out = _check_output(['ip', '-6', 'ro', 'list']).decode("utf-8")
    for line in ip_out.split('\n'):
        route = line.split()
        if route[0] == "default" and len(route) >= 5:
            return {"default_iface": route[4].strip()}
    return {}

