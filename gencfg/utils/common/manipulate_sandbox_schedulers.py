#!/skynet/python/bin/python
"""
    Perform various actions with sandbox schedulers:
        - list of current schedulers with statuses
        - stop schedulers
        - start schedulers
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import json
import time
import urllib2

import gencfg
from core.argparse.parser import ArgumentParserExt
import core.argparse.types
from core.settings import SETTINGS
from gaux.aux_utils import retry_urlopen, request_sandbox
from gaux.aux_colortext import green_text, red_text, yellow_text
from dateutil import parser as dateutil_parser

FMT = "%Y-%m-%dT%H:%M:%SZ"
DAY = 60 * 60 * 24


class EActions(object):
    SHOW = "show"
    LASTOK = "lastok"  # get time of last success task
    START = "start"
    STOP = "stop"
    ALL = [SHOW, LASTOK, START, STOP]


def set_scheduler_start_time(scheduler_json, oauth_token):
    if scheduler_json["schedule"]["start_time"] is None:  # scheduler without start time specified
        return

    request_json = scheduler_json["schedule"]

    startt = int(time.mktime(time.strptime(request_json["start_time"], FMT)))
    nowt = int(time.time())
    new_startt = ((nowt - startt) / DAY) * DAY + DAY + startt
    request_json["start_time"] = time.strftime(FMT, time.localtime(new_startt))

    path = "/scheduler/{}".format(scheduler_json["id"])
    data = json.dumps({"schedule": request_json})
    headers = {
        'Content-Type': 'application/json',
        'Authorization': 'OAuth {}'.format(options.oauth_token)
    }

    request_sandbox(path, data=data, headers=headers, method='PUT')


def color_status(status):
    if status == "SUCCESS":
        return green_text(status)
    elif status == "WAITING":
        return yellow_text(status)
    else:
        return red_text(status)


def get_parser():
    parser = ArgumentParserExt(description="Perform various actions with sandbox schedulers")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=EActions.ALL,
                        help="Obligatory. Action to execute")
    parser.add_argument("-t", "--task-type", type=str, default=None,
                        help="Optional. Perform action on schedulers of specified type")
    parser.add_argument("--schedulers", type=core.argparse.types.comma_list, default=None,
                        help="Optional. Perform actions with only with specified schedulers")
    parser.add_argument('--oauth-token', type=str, default=None,
                        help='Optional. Oauth token for actions, that modify schedulers: {}'.format(','.join([EActions.START, EActions.STOP])))

    return parser


def normalize(options):
    if (options.task_type is not None) and (options.schedulers is not None):
        raise Exception('Options <--task-type> and <--schedulers> are mutually exclusive')

    if options.schedulers is not None:
        options.schedulers = map(int, options.schedulers)

    if options.action in (EActions.START, EActions.STOP):
        if options.oauth_token is None:
            raise Exception('You have to specify <--oauth-token> with action <{}>'.format(options.action))


def get_sandbox_finish_time_status(task_info):
    if task_info['status'] == 'SUCCESS':
        status = True
    else:
        status = False

    if 'execution' in task_info and 'started' in task_info['execution']:
        lastok = task_info['execution']['started']
        lastok = lastok.partition('.')[0]
        if lastok.endswith('Z'):
            lastok = lastok[:-1]
        lastok = int(time.mktime(time.strptime(lastok, '%Y-%m-%dT%H:%M:%S')))
        lastok = lastok - time.timezone
    else:
        lastok = None

    return status, lastok


def get_scheduler_lastok(scheduler_id, headers=None):
    tasks = request_sandbox('/task?scheduler={}&limit=100'.format(scheduler_id), headers=headers)['items']

    task_statuses = [get_sandbox_finish_time_status(task) for task in tasks]
    good_finish_times = [y for (x, y) in task_statuses if x is True]

    if not good_finish_times:
        return None

    scheduler_lastok = max(good_finish_times)

    return scheduler_lastok


def get_scheduler_description(scheduler_id):
    scheduler_info_url = '{}/scheduler/{}'.format(SETTINGS.services.sandbox.rest.url, scheduler_id)
    scheduler_info = json.loads(retry_urlopen(3, scheduler_info_url))

    return scheduler_info['task']['description']


def main_lastok(options):
    if options.schedulers is not None:
        scheduler_ids = options.schedulers
    else:
        scheduler_ids = [x['id'] for x in request_sandbox('/scheduler?task_type={}&limit=100'.format(options.task_type))['items']]

    for scheduler_id in scheduler_ids:
        scheduler_lastok = get_scheduler_lastok(scheduler_id)

        if scheduler_lastok is None:
            continue

        lastok_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(scheduler_lastok))
        scheduler_description = get_scheduler_description(scheduler_id)

        print 'Scheduler {} ({}): last ok status {}'.format(scheduler_id, scheduler_description, lastok_str)


def main(options):
    if options.task_type is not None:
        data = request_sandbox('/scheduler?task_type={}&limit=1000'.format(options.task_type))
        print "Found %d schedulers of type %s:" % (data["total"], options.task_type)
    elif options.schedulers is not None:
        data = []
        for scheduler_id in options.schedulers:
            data.extend(request_sandbox('/scheduler?id={}&limit=1'.format(scheduler_id))['items'])
        data = dict(items=data)

    if options.action == EActions.SHOW:
        for scheduler in data["items"]:
            print "    Scheduler %s (by %s):" % (scheduler["id"], scheduler["author"])
            print "        Descr: %s" % (scheduler["task"]["description"])

            status = scheduler["task"]["last"]["status"]
            status = color_status(status)
            print "        Status: %s" % status

            # add information on last tasks
            last_tasks = request_sandbox('/task?scheduler={}&limit=30'.format(scheduler["id"]))
            print "        Last tasks:"
            for task in last_tasks["items"]:
                status = color_status(task["status"])
                finished = dateutil_parser.parse(task["time"]["updated"]).timetuple()
                finished = time.strftime("%Y-%m-%d %H:%M:%S", finished)
                print "             Task %s: %s (%s)" % (task["id"], status, finished)
    elif options.action == EActions.LASTOK:
        main_lastok(options)
    elif options.action == EActions.START:
        for scheduler in data["items"]:
            set_scheduler_start_time(scheduler, options.oauth_token)

            path = "/batch/schedulers/start"
            data = json.dumps([scheduler["id"]])
            headers = {
                'Content-Type': 'application/json',
                'Authorization': 'OAuth {}'.format(options.oauth_token)
            }
            request_sandbox(path, data=data, headers=headers, method='PUT')

            print "    Scheduler %s (by %s): STARTED" % (scheduler["id"], scheduler["author"])
    elif options.action == EActions.STOP:
        for scheduler in data["items"]:
            path = "/batch/schedulers/stop"
            data = json.dumps([scheduler["id"]])
            headers = {
                'Content-Type': 'application/json',
                'Authorization': 'OAuth {}'.format(options.oauth_token)
            }
            request_sandbox(path, data=data, headers=headers, method='PUT')

            print "    Scheduler %s (by %s): STOPPED" % (scheduler["id"], scheduler["author"])
    else:
        raise Exception("Unknown action %s" % options.action)


if __name__ == "__main__":
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
