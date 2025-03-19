#!/usr/bin/env python
""" HBF monitoring script """
import sys
import time
import requests


URI = "http://localhost:9876/status"
URI_TIMEOUT = 1
TIMEWINDOW = 600
FUTUREWINDOW = -1


def get_status(url, timeout):
    """
    Get current agent status from api
    :param url: agent status url
    :param timeout: timeout for request
    :return: agent status, dict
    """
    status = requests.get(url, timeout=timeout)
    status.raise_for_status()
    return status.json()


def monrun(lvl, msg=None, ecode=0):
    """
    Print message in monrun format and exit
    :param lvl: monrun level, string, variants: ['ok','warn','crit']
    :param msg: monrun message, string
    :param ecode: exit code, int. If None - do not exit
    :return: None
    """
    _possible_levels = {'ok': 0, 'warn': 1, 'crit': 2}
    if lvl not in _possible_levels:
        raise Exception('No such monrun level: %s. Possible values: %s' % (
            lvl, _possible_levels.keys()))
    print('%s; %s' % (_possible_levels[lvl], msg if msg else lvl))
    if ecode is not None: sys.exit(ecode)


def main():
    """ Fire! """
    data = None
    ts_now = int(time.time())
    ts_agent = None

    # Get agent status
    try:
        data = get_status(URI, URI_TIMEOUT)
    except (requests.exceptions.ConnectionError,
            requests.exceptions.HTTPError) as error:
        monrun('crit', 'Agent status api not available: %s' % error)
    except ValueError:
        monrun('crit', 'Couldnt parse agent status api answer')

    try:
        ts_agent = int(data['last_update'])
    except KeyError:
        monrun('crit', 'No last_update field in agent status api answer')

    ts_delta = ts_now - ts_agent

    # Run check
    if ts_delta < FUTUREWINDOW:
        monrun('crit', 'Last update ts from future. Agent ts: %s, now: %s' % (
            ts_agent, ts_now))
    elif ts_delta > TIMEWINDOW:
        monrun('crit',
               'Last update ts is too old. Agent ts: %s, now: %s, delta: %s. '
               % (ts_agent, ts_now, ts_delta) +
               'Agent status: %s. Agent status desc: %s' % (
                   data.get('status'), data.get('desc')))
    else:
        monrun('ok', 'Last update: %ss ago, threshold: %ss' % (ts_delta,
                                                               TIMEWINDOW))


if __name__ == '__main__':
    main()
