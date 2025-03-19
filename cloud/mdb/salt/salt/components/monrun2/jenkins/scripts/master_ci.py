#!/usr/bin/env python
"""
Monitoring for MDB CI on trunk
"""

import sys

import requests


def die(status=0, message='OK'):
    """
    Report status to juggler
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def all_jobs_status(url, failed_build):
    """
    Check master-ci run with ctype filter
    """
    ret = requests.get('http://localhost:8080/job/master-ci/{number}/logText/progressiveText?start=0'.format(
        number=failed_build)).text
    has_passed = False
    for line in ret.splitlines():
        if line.startswith('Passed ') and 'job/arcadia_infrastructure_' in line:
            has_passed = True
            break
    if not has_passed:
        die(2, 'All jobs failed for master-ci build: {url}'.format(url=url))

    die()


def ctype_status(ctype, failed_build):
    """
    Check master-ci run with ctype filter
    """
    ret = requests.get('http://localhost:8080/job/master-ci/{number}/logText/progressiveText?start=0'.format(
        number=failed_build)).text
    failed = []
    for line in ret.splitlines():
        if line.startswith('Failed ') and 'job/arcadia_infrastructure_' in line:
            job_ctype = line.split('/')[4].split('_')[2]
            if job_ctype == ctype:
                failed.append(line.split()[-1])
    if not failed:
        die()

    die(1, '{count} jobs failed: {urls}'.format(count=len(failed), urls=', '.join(failed)))


def _main():
    try:
        ctype = None
        if len(sys.argv) == 2:
            ctype = sys.argv[1]
        job = requests.get('http://localhost:8080/job/master-ci/api/json').json()
        last_complete = job['lastCompletedBuild']
        last_failed = job['lastFailedBuild']
        if last_complete['number'] == last_failed['number']:
            if not ctype:
                all_jobs_status(last_failed['url'], last_failed['number'])
            else:
                ctype_status(ctype, last_failed['number'])
        die()
    except Exception as exc:
        die(1, 'Unable to check trunk status: {error}'.format(error=repr(exc)))


if __name__ == '__main__':
    _main()
