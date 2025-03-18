#!/skynet/python/bin/python
"""
    This script update table with information on commits (all this information can be found in sandbox, receiving this info is very slow).
    This table is ALL_HEARTBEAT_C_MONGODB
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import xmlrpclib
import pymongo
import pprint
import json

import gencfg
from core.argparse.parser import ArgumentParserExt
import core.argparse.types
from gaux.aux_mongo import get_mongo_collection
from core.db import CURDB
from config import GENCFG_TRUNK_DATA_PATH
from core.settings import SETTINGS
from gaux.aux_utils import OAuthTransport, indent, request_sandbox


class EActions(object):
    SHOW = "show"
    UPDATE = "update"
    ALL = [SHOW, UPDATE]


def get_parser():
    parser = ArgumentParserExt("Script to show/update commits info")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=EActions.ALL,
                        help="Obligatory. Action to execute")
    parser.add_argument("--update-db", action="store_true", default=False,
                        help="Optional. Update mongo table in action <%s>, thus updating cache before getting data" % (
                        EActions.SHOW))
    parser.add_argument("-l", "--limit", type=int, default=10,
                        help="Optional. Option for action <%s>: show only <--limit> commits" % EActions.SHOW)
    parser.add_argument("--offset", type=int, default=0,
                        help="Optional. Option for action <%s>: show commits with specified offset" % EActions.SHOW)
    parser.add_argument("--start-commit", type=int, default=None,
                        help="Optional. Option for action <%s>: show commits starting with specified one" % (
                        EActions.SHOW))
    parser.add_argument("--author", type=str, default=None,
                        help="Optional. Option for action <%s>: show commits of specified author only" % (
                        EActions.SHOW))
    parser.add_argument("--modifies", type=core.argparse.types.comma_list, default=None,
                        help="Optional. Option for action <%s>: show only commits, which modify one of comma-separated specified file" % (
                        EActions.SHOW))
    parser.add_argument("--labels", type=core.argparse.types.comma_list, default=None,
                        help="Optional. Option for action <%s>: show only commits with one of comma-separated specified label (label is something in square braces)" % (
                        EActions.SHOW))
    parser.add_argument("-f", "--filter", type = core.argparse.types.pythonlambda, default=lambda x: True,
                        help="Optional. Extra filter on commit info dict . E. g. (to filter only tasks with broken goals): <lambda x: 'testinfo' in x and 'broken_goals' in x['testinfo'] and len(x['testinfo']['broken_goals']) > 0>")

    return parser


def extract_commit_info_from_commit(commit):
    """
        Extract all data from commit info and save in structure for export into mongo

        :type commit: core.svnapi.SvnCommitMessage

        :param commit: commit to process
        :return dict: dict of what will be exported to mongo
    """

    return {
        "id": commit.commit,
        "author": commit.author,
        "date": commit.date,
        "message": commit.message,
        "mtags": commit.labels,
        "modified_files": commit.modified_files,
    }


def extract_commit_info_from_task(taskid):
    """
        Extract all data from sandbox test task

        :type taskid: int

        :param taskid: existing sandbox task
        :return dict: dict of what will be exported to mongo
    """

    task = request_sandbox('/task/{}'.format(taskid))

    result = {
        "taskid": taskid,
    }

    # fill status field
    if task["status"] in ["FAILURE", "UNKNOWN", "STOPPED", "TIMEOUT", "EXCEPTION"]:
        result["status"] = "BROKEN"
    elif task["status"] == "SUCCESS":
        task_context = request_sandbox('/task/{}/context'.format(taskid))
        result["status"] = task_context["build_status"]
    else:
        result["status"] = "SCHEDULED"

    if task["status"] == "SUCCESS":
        result["diff"] = task_context.get("diff_builder_result", "")

        # fill broken goals links
        result["broken_goals"] = []
        build_log_resources = request_sandbox('/resource?type=CONFIG_BUILD_LOGS&limit=1&task_id={}'.format(task['id']))
        if len(build_log_resources) == 1:
            build_log_resource = build_log_resources[0]
            build_log_resource_url = build_log_resource['proxy_url']
            for broken_goal in task_context.get('build_broken_goals', []):
                result["broken_goals"].append({
                    "goal": broken_goal.partition('/')[2],
                    "proxy_url": "%s/%s.log" % (build_log_resource_url, broken_goal.partition('/')[2])
                })

    return result


def get_unchecked_sandbox_tasks(taskid):
    """
        Get list of yet unchecked tasks

        :type taskid: int

        :param taskid: taskid of existing sandbox task
        :return (int, list): pair of <new unchecked task id>, <list of sandbox tasks newer than taskid>
    """

    found = False
    limit = 5
    while not found:
        tasks = request_sandbox('/task?type=TEST_CONFIG_GENERATOR&limit={}'.format(limit))['items']

        if len(filter(lambda x: x["id"] < taskid, tasks)) == 0:
            limit *= 2
            continue
        else:
            found = True

        tasks = filter(lambda x: x["id"] > taskid, tasks)
        tasks.sort(cmp=lambda x, y: cmp(x["id"], y["id"]))

        for task in tasks:
            if task["status"] in ["SUCCESS", "FAILURE", "TIMEOUT", "EXCEPTION", "UNKNOWN", "DRAFT", "STOPPED"]:
                taskid = task["id"]
            else:
                break

    return taskid, tasks


def updatedb(options):
    """
        Update ALL_HEARTBEAT_C_MONGODB.topology_commits.gencfgcommits table with info on commits. Perform following actions
          - get all commits from svn, which are not in db and add them to db
          - get all sandbox tasks up to last unchecked and update their info in db
    """
    del options
    mongocoll = get_mongo_collection('gencfgcommits')
    mongostatecoll = get_mongo_collection('gencfgstate')

    # first insert into table commits which are not in table
    last_commit_in_db = list(mongocoll.find().sort("commit.id", pymongo.DESCENDING))[0]["commit"]["id"]

    commits_to_insert = CURDB.get_repo().get_commits_since(last_commit_in_db, remote_url=GENCFG_TRUNK_DATA_PATH)
    for commit in commits_to_insert:
        mongocoll.remove({"commit.id": commit.commit})
        mongocoll.insert({"commit": extract_commit_info_from_commit(commit)})
    commits_to_insert = commits_to_insert[1:]

    # than go and get data from unchecked sandbox tasks
    last_processed_task = int(mongostatecoll.find_one({"id": "util.update_commits_info.last_task"})["value"])
    new_last_processed_task, tasks_to_insert = get_unchecked_sandbox_tasks(last_processed_task)

    for task in tasks_to_insert:
        commit = task["ctx"]["last_commit"]
        commit_data = mongocoll.find_one({"commit.id": commit}, {"_id": False})
        if not commit_data:
            continue
        commit_data.update({"testinfo": extract_commit_info_from_task(task["id"])})
        mongocoll.update({"commit.id": commit}, commit_data)

    mongostatecoll.update(
        {"id": "util.update_commits_info.last_task"},
        {"id": "util.update_commits_info.last_task", "value": new_last_processed_task}
    )

    result = {
        "commits": commits_to_insert,
        "tasks": tasks_to_insert,
    }

    return result


def showdb(options):
    mongocoll = get_mongo_collection('gencfgcommits')

    if options.update_db:
        updatedb(options)

    # add filters
    filters = {}
    if options.author is not None:
        filters["commit.author"] = options.author

    mongoiter = mongocoll.find(filters).sort("commit.id", pymongo.DESCENDING)

    cur_id = -1
    first_id_to_return = options.offset
    last_id_to_return = options.offset + options.limit
    result = []
    for elem in mongoiter:
        elem.pop("_id")
        if options.modifies is not None:
            found = True
            for fname in options.modifies:
                if len(filter(lambda x: x.find(fname) >= 0, elem["commit"]["modified_files"])) > 0:
                    found = True
                    break
            if not found:
                continue

        if options.labels is not None:
            if len(set(options.labels) & set(elem["commit"]["mtags"])) == 0:
                continue

        cur_id += 1

        if options.start_commit is None:
            if first_id_to_return <= cur_id < last_id_to_return:
                result.append(elem)

            if cur_id >= last_id_to_return:
                break
        else:
            result.append(elem)
            if options.start_commit >= elem["commit"]["id"]:
                break

    result = filter(options.filter, result)

    return result


def main(options):
    if options.action == EActions.SHOW:
        result = showdb(options)
    elif options.action == EActions.UPDATE:
        result = updatedb(options)
    else:
        raise Exception("Unknown action <%s>" % options.action)

    return result


def normalize(options):
    del options
    pass


def commit_to_str(commit):
    commit_id = commit["commit"]["id"]
    author = commit["commit"]["author"]
    message = commit["commit"]["message"][:300]
    modified_files = ",".join(commit["commit"]["modified_files"])
    if "testinfo" in commit:
        sandbox_status = commit["testinfo"]["status"]
    else:
        sandbox_status = "NOTSTARTED"

    result = """Commit %(commit_id)s (by %(author)s):
    Message: <%(message)s>
    Modifies: <%(modified_files)s>
    Test status: <%(sandbox_status)s>""" % {
        "commit_id": commit_id,
        "author": author,
        "message": message,
        "modified_files": modified_files,
        "sandbox_status": sandbox_status,
    }

    if "testinfo" in commit and "broken_goals" in commit["testinfo"]:
        broken_goals_info = ["    Broken goals: (task %s%s)" % (SETTINGS.services.sandbox.http.tasks.url, commit["testinfo"]["taskid"])]
        for broken_goal in commit["testinfo"]["broken_goals"]:
            broken_goals_info.append("       %s: %s" % (broken_goal["goal"], broken_goal["proxy_url"]))
        result = "%s\n%s" % (result, "\n".join(broken_goals_info))

    return result


def print_result(result, options):
    if options.action == EActions.SHOW:
        print "Filtered %d commits" % len(result)
        for commit in result:
            print indent(commit_to_str(commit).encode('utf8'))
    elif options.action == EActions.UPDATE:
        print "Inserted %d commits: %s" % (
        len(result["commits"]), ",".join(map(lambda x: str(x.commit), result["commits"])))
        print "Updated %d tasks: %s" % (len(result["tasks"]), ",".join(map(lambda x: str(x["id"]), result["tasks"])))
    else:
        raise Exception("Unknown action <%s>" % options.action)


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options)

    print_result(result, options)
