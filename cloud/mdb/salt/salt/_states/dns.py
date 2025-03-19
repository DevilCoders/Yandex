# coding: utf-8
import time

__opts__ = {}
__salt__ = {}
__pillar__ = {}


def __virtual__():
    return True


def wait_for_ipv4(name, fqdn, wait_timeout=600):
    ret = {'name': name, 'result': False, 'changes': {}, 'comment': ''}

    if __opts__['test']:
        ret['result'] = True
        return ret

    deadline = time.time() + wait_timeout
    while time.time() <= deadline:
        if __salt__['dns.has_ipv4_address'](fqdn=fqdn):
            ret['result'] = True
            return ret

        time.sleep(5)

    ret['comment'] = "Can't resolve fqdn {0} by ipv4 for timeout {1} seconds".format(fqdn, wait_timeout)
    return ret


def wait_for_ipv6(name, fqdn, wait_timeout=600):
    ret = {'name': name, 'result': False, 'changes': {}, 'comment': ''}

    if __opts__['test']:
        ret['result'] = True
        return ret

    deadline = time.time() + wait_timeout
    while time.time() <= deadline:
        if __salt__['dns.has_ipv6_address'](fqdn=fqdn):
            ret['result'] = True
            return ret

        time.sleep(5)

    ret['comment'] = "Can't resolve fqdn {0} by ipv6 for timeout {1} seconds".format(fqdn, wait_timeout)
    return ret
