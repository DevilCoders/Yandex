#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import pwd
import json
import urllib2

import gencfg
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from core.settings import SETTINGS

class EActions(object):
    CREATE = "create"
    CREATERCCS = "createrccs"
    ALL = [CREATE, CREATERCCS]

def get_parser():
    parser = ArgumentParserExt(description="Create startrek task from console")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=EActions.ALL,
                        help="Obligatory. Action to execute")
    parser.add_argument("-q", "--queue", type=str, default=None,
                        help="Optional. Startrek queue name (for actions <%s>)" % EActions.CREATE)
    parser.add_argument("-s", "--summary", type=str, default=None,
                        help="Optional. Task name via short one line description (for actions <%s>)" % EActions.CREATE)
    parser.add_argument("-d", "--description", type=argparse_types.floaded_str, default=None,
                        help="Optional. Detailed task description (for actions <%s>)" % EActions.CREATE)
    parser.add_argument("-e", "--assignee", type=str, required=True,
                        help="Obligatory. Assign task to specified user")
    parser.add_argument("-f", "--followers", type=argparse_types.comma_list, default=[],
                        help="Optional. List of followers")
    parser.add_argument("-p", "--parent-task", type=str, default=None,
                        help="Optional. Parent task")
    parser.add_argument("-g", "--group", type=argparse_types.group, default=None,
                        help="Optional. Created RCCS task for specified group (for actions <%s>)" % EActions.CREATERCCS)

    return parser


def create_task(options):
    if options.parent_task is not None:
        # check if parent task exists
        task_url = "%sv2/issues/%s" % (SETTINGS.services.startrek.rest.url, options.parent_task)
        headers = {"Authorization": "OAuth %s" % SETTINGS.services.startrek.rest.oauth,}
        check_req = urllib2.Request(task_url, None, headers)
        try:
            # noinspection PyUnusedLocal
            resp = urllib2.urlopen(check_req)
        except urllib2.HTTPError, e:
            if e.code in [400, 404]:
                raise Exception("Parent task <%s> does not exists" % options.parent_task)
            else:
                raise Exception("Got unknown exception while checking if parent task <%s> exists: <%s>" % (options.parent_task, str(e)))
        except Exception, e:
            raise Exception("Got unknown exception while checking if parent task <%s> exists: <%s>" % (options.parent_task, str(e)))

    url = "%sv2/issues" % SETTINGS.services.startrek.rest.url
    headers = {
        "Authorization": "OAuth %s" % SETTINGS.services.startrek.rest.oauth,
        "Content-Type": "application/json",
    }

    data = {
        "queue": options.queue,
        "summary": options.summary,
        "description": options.description,
        "assignee": options.assignee,
        "followers": list(set(options.followers + [pwd.getpwuid(os.getuid())[0]])),
    }
    if options.parent_task is not None:
        data["parent"] = options.parent_task
    data = json.dumps(data)

    req = urllib2.Request(url, data, headers)
    try:
        resp = urllib2.urlopen(req)
    except Exception, e:
        raise Exception("Got exception while creating new startrek task: <%s>" % (str(e)))

    response = json.loads(resp.read())
    return response


def create_rccs_task(options):
    url = "%sv2/issues" % SETTINGS.services.startrek.rest.url
    headers = {
        "Authorization": "OAuth %s" % SETTINGS.services.startrek.rest.oauth,
        "Content-Type": "application/json",
    }

    summary = "Move hosts to non-search group %s" % options.group.card.name
    hostnames = " ".join(sorted(map(lambda x: x.name, options.group.getHosts())))
    description = "Hosts from specified group are about to move from search to non-search project. Thus we have to remove this hosts from walle.\n\nFull list of hosts: %s" % hostnames

    data = {
        "queue": "RCCSADMIN",
        "summary": summary,
        "description": description,
        "assignee": options.assignee,
        "followers": list(set(options.followers + options.group.card.owners + [pwd.getpwuid(os.getuid())[0]])),
    }
    if options.parent_task is not None:
        data["parent"] = options.parent_task
    data = json.dumps(data)

    req = urllib2.Request(url, data, headers)
    try:
        resp = urllib2.urlopen(req)
    except Exception, e:
        raise Exception("Got exception while creating new startrek task: <%s>" % (str(e)))

    response = json.loads(resp.read())
    return response


def main(options):
    if options.action == EActions.CREATE:
        return create_task(options)
    elif options.action == EActions.CREATERCCS:
        return create_rccs_task(options)
    else:
        raise Exception("Unknown action <%s>" % options.action)


def jsmain(d):
    options = get_parser().parse_json(d)
    return main(options)


def print_result(result):
    print "Created startrek task: %s%s" % (SETTINGS.services.startrek.http.url, result["key"])


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    result = main(options)

    print_result(result)
