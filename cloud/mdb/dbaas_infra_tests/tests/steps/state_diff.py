"""
Steps related to salt highstate diff tests.
"""

import os
import time
from multiprocessing.pool import ThreadPool

from behave import then, when

from tests.helpers import docker


def get_highstate_test_out(container):
    """
    Run highstate with test=True in container and get output
    """
    _, sync_out = container.exec_run('salt-call saltutil.sync_all')
    _, output = container.exec_run(
        'salt-call --out=highstate --output-diff --state-out=changes --log-level=quiet state.highstate test=True')
    return sync_out.decode() + output.decode()


def get_highstate_out(container):
    """
    Run highstate in container and get output
    """
    _, output = container.exec_run(
        'salt-call --out=highstate --output-diff --state-out=changes --log-level=quiet state.highstate queue=True')
    return output.decode()


def _get_highstate_outs(context, highstate_fun, zone_name=None):
    pool = ThreadPool(processes=len(context.hosts))
    jobs = {}
    if zone_name:
        hosts = [host for host in context.hosts if host['zoneId'] == zone_name]
    else:
        hosts = context.hosts
    for host in hosts:
        short_name, *_ = host['name'].split('.')
        container = docker.get_container(context, short_name)
        job = pool.apply_async(highstate_fun, (container, ))
        jobs[host['name']] = job

    deadline = time.time() + 1200

    while time.time() < deadline:
        for host, job in jobs.copy().items():
            if not job.ready():
                continue
            output = job.get()
            del jobs[host]
            yield host, output
        if not jobs:
            return
        time.sleep(1)

    if jobs:
        assert False, 'Highstate with test=True hang on hosts: {hosts}'.format(hosts=', '.join(jobs))


@when('we run highstate on all hosts in cluster')
def run_highstate(context):
    """
    Check if highstate on all cluster hosts
    """
    for host, output in _get_highstate_outs(context, get_highstate_out):
        for line in output.splitlines():
            if line.startswith('Failed:') and line.split()[1] != '0':
                assert False, 'Failed states on {host} in highstate call:\n{out}'.format(host=host, out=output)


@when('we run highstate on all hosts from zone "{zone_name:Param}"')
def run_highstatei_from_zone(context, zone_name):
    """
    Check if highstate on all cluster hosts from specified zone
    """
    for host, output in _get_highstate_outs(context, get_highstate_out, zone_name):
        for line in output.splitlines():
            if line.startswith('Failed:') and line.split()[1] != '0':
                assert False, 'Failed states on {host} in highstate call:\n{out}'.format(host=host, out=output)


@then('cluster has no pending changes')
def check_highstate_changes(context):
    """
    Check if highstate with test=True gives changes on any cluster host
    """
    forced_by_summary = '[diff-check]' in os.environ.get('SUMMARY', '')
    is_trunk = os.environ.get('ARCANUMID', '') == 'trunk'
    forced_by_config = context.conf.get('force_diff', False)
    if not forced_by_summary and not is_trunk and not forced_by_config:
        return
    for host, output in _get_highstate_outs(context, get_highstate_test_out):
        for line in output.splitlines():
            if line.startswith('Succeeded:') and 'changed=' in line:
                assert False, 'Unexpected diff on {host} in highstate call:\n{out}'.format(host=host, out=output)
            elif line.startswith('Failed:') and line.split()[1] != '0':
                assert False, 'Failed states on {host} in highstate call:\n{out}'.format(host=host, out=output)
