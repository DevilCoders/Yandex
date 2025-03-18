#!/skynet/python/bin/python
"""Calculate data for kpi graphics"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import sqlite3
import xmlrpclib
import time
import functools
from collections import defaultdict
import json
import datetime

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
from core.settings import SETTINGS
from gaux.aux_utils import OAuthTransport, retry_urlopen
from core.svnapi import SvnRepository
import utils.mongo.update_commits_info as update_commits_info
import core.argparse.types as argparse_types
from gaux.aux_clickhouse import run_query as run_clickhouse_query
from gaux.aux_clickhouse import EBaseId
import pytz

from utils.common.manipulate_sandbox_schedulers import get_scheduler_lastok, get_scheduler_description


def get_db_connection():
    """
        Get connection to database as object
    """
    return sqlite3.connect(os.path.join(CURDB.get_path(), "kpi.sqlite"))


class IUpdater(object):
    """
        Abstract class to update kpi graph
    """

    def __init__(self, signal_name, signal_func, signal_comment_func):
        self.signal_name = signal_name
        self.signal_func = signal_func
        self.signal_comment_func = signal_comment_func

        self.conn = get_db_connection()

        self.old_points = []
        self.new_points = []

    def gen_new_points(self, options):
        """
            Generate extra data to be inserted into kpi table
        """
        raise NotImplementedError("Method <gen_new_points> is not implemented")

    def show_new_points(self):
        print "Signal <%s>: add <%d> points" % (self.signal_name, len(self.new_points))
        for commit_id, commit_timestamp, value, comment in self.new_points:
            print "    Commit <%s> (started <%s>): %f" % (commit_id, time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(commit_timestamp)), value)

    def add_new_points(self):
        if options.verbose > 0:
            print "Signal <%s>: add <%d> points" % (self.signal_name, len(self.new_points))
        for commit_id, commit_timestamp, value, comment in self.new_points:
            event_date = datetime.datetime.fromtimestamp(int(time.time()), tz=pytz.timezone("Europe/Moscow")).strftime('%Y-%m-%d')
            if commit_id is not None:
                normalized_commit_id = commit_id
            else:
                normalized_commit_id = 'null'
            query = "INSERT INTO kpi_graphs_data values ('{event_date}', '{signal_name}', {commit_id}, {commit_timestamp}, {value}, '{comment}')".format(
                    event_date=event_date, signal_name=self.signal_name, commit_id=normalized_commit_id, commit_timestamp=commit_timestamp, value=float(value),
                    comment=comment)
            print 'Query {}'.format(query)
            run_clickhouse_query(query, base_id=options.base_id)


class TSandboxTaskUpdater(IUpdater):
    """
        Class, that updates info on sandbox task
    """

    def __init__(self, signal_name, signal_func, signal_comment_func, task_type):
        """
            Initialize updater

            :param task_type: sandbox task type
            :param signal_name: signal name (should be uniq)
            :param signal_func: fucntion, which generates value to graph
            :param signal_comment_func: function, which generates comment to graph point
        """

        super(TSandboxTaskUpdater, self).__init__(signal_name, signal_func, signal_comment_func)

        self.task_type = task_type

    def gen_new_points(self, options):
        # first get last point id stored into db
        result = run_clickhouse_query("SELECT point_id FROM kpi_graphs_data WHERE signal_name = '{}' ORDER by timestamp DESC LIMIT 1".format(self.signal_name),
                                      base_id=options.base_id)
        if len(result):
            last_processed_task = int(result[0][0])
        else:
            last_processed_task = 0

        if options.verbose > 1:
            print 'TSandboxTaskUpdater: last processed task {}'.format(last_processed_task)

        # get list of tasks which are newer the
        tasks_url = '{}/task?type={}&limit=100'.format(SETTINGS.services.sandbox.rest.url, self.task_type)

        if options.verbose:
            print 'Requesting task list: {}'.format(tasks_url)

        tasks = json.loads(retry_urlopen(3, tasks_url))['items']
        tasks = filter(lambda x: x["id"] > last_processed_task, tasks)

        # get points from task
        for task in tasks:
            signal_value = self.signal_func(task)
            if signal_value is None: # some tasks can be "broken" and can not be shown on graphs (FAILURE tasks are broken for <task duration> signal)
                continue

            new_point = task["id"], convert_sandbox_time(task['execution']['started']), signal_value, self.signal_comment_func(task)
            self.new_points.append(new_point)

            if options.verbose > 1:
                print 'TSandboxTaskUpdater: added point {}'.format(new_point)


class TCommitUpdater(IUpdater):
    def __init__(self, signal_name, signal_func, signal_comment_func):
        """
            Initialize updater

            :param signal_name: signal name (should be uniq)
            :param signal_func: function, which generates value to graph
            :param signal_comment_func: function, which genrates comment for specific point
        """

        super(TCommitUpdater, self).__init__(signal_name, signal_func, signal_comment_func)

    def gen_new_points(self, options):
        # first get last point id stored into db
        result = run_clickhouse_query("SELECT point_id, value FROM kpi_graphs_data WHERE signal_name = '{}' ORDER by timestamp DESC LIMIT 1".format(self.signal_name),
                                      base_id=options.base_id)
        if len(result):
            last_processed_commit, last_processed_value = int(result[0][0]), int(result[0][1])
        else:
            last_processed_commit, last_processed_value = 2248022, 0

        if options.verbose:
            print 'TCommitUpdater: last processed commit <{}, {}>'.format(last_processed_commit, last_processed_value)

        myrepo = SvnRepository(CURDB.get_path())
        commits = myrepo.get_commits_since(last_processed_commit + 1)

         # get points from task
        for commit in sorted(commits, key=lambda x: x.date):
            signal_value = self.signal_func(commit)
            if signal_value is None: # some tasks can be "broken" and can not be shown on graphs (FAILURE tasks are broken for <task duration> signal)
                continue
            last_processed_value += signal_value

            self.new_points.append((commit.commit, int(commit.date), last_processed_value, self.signal_comment_func(commit)))


class TCommitGreenLevelUpdater(IUpdater):
    def __init__(self, signal_name):
        super(TCommitGreenLevelUpdater, self).__init__(signal_name, None, None)

    def gen_new_points(self, options):
        # first get last point id stored into db
        result = run_clickhouse_query("SELECT point_id, value FROM kpi_graphs_data WHERE signal_name = '{}' ORDER by timestamp DESC LIMIT 1".format(self.signal_name),
                                      base_id=options.base_id)
        if len(result):
            last_processed_commit, last_processed_value = int(result[0][0]), float(result[0][1])
        else:
            last_processed_commit, last_processed_value = 2311481, 1.

        if options.verbose:
            print 'TCommitGreenLevelUpdater: last processed commit <{}, {}>'.format(last_processed_commit, last_processed_value)

        commits = list(reversed(update_commits_info.jsmain({ "action" : "show", "start_commit" : last_processed_commit})))

        curvalue = last_processed_value
        curdate = commits[0]["commit"]["date"]
        for commit in commits[1:]:
            if "testinfo" not in commit:
                continue
            if commit["testinfo"]["status"] not in ["SUCCESS", "FAILURE"]:
                continue

            newdate = commit["commit"]["date"]
            coeff = (newdate - curdate) / (60. * 60 * 24 * 7)
            if commit["testinfo"]["status"] == "SUCCESS":
                newvalue = curvalue * (1 - coeff) + coeff
            elif commit["testinfo"]["status"] == "FAILURE":
                newvalue = curvalue * (1 - coeff)

            curdate = newdate
            curvalue = newvalue

            self.new_points.append((commit["commit"]["id"], int(curdate), curvalue, None))


class TCommitGencfgNotWorking(IUpdater):
    def __init__(self, signal_name):
        super(TCommitGencfgNotWorking, self).__init__(signal_name, None, None)

    def gen_new_points(self, options):
        # first get last point id stored into db
        result = run_clickhouse_query("SELECT point_id, value FROM kpi_graphs_data WHERE signal_name = '{}' ORDER by timestamp DESC LIMIT 1".format(self.signal_name),
                                      base_id=options.base_id)
        if len(result):
            if result[0][0] == '\\N':
                last_processed_commit, curvalue = None, int(result[0][1])
            else:
                last_processed_commit, curvalue = int(result[0][0]), int(result[0][1])
        else:
            last_processed_commit, curvalue = 2713320, 0

        if options.verbose:
            print 'TCommitGreenLevelUpdater: last processed commit <{}, {}>'.format(last_processed_commit, curvalue)

        commits = list(reversed(update_commits_info.jsmain({ "action" : "show", "start_commit" : last_processed_commit})))

        curcommit = commits[0]
        curdate = commits[0]["commit"]["date"]
        for commit in commits[1:]:
            if "testinfo" not in commit:
                continue
            if commit["testinfo"]["status"] not in ["SUCCESS", "FAILURE"]:
                continue

            newdate = commit["commit"]["date"]
            if commit["testinfo"]["status"] == "SUCCESS":
                newvalue = 0
            else:
                newvalue = 1

            if curvalue != newvalue:
                self.new_points.append((None, int(curdate) + 1, newvalue, None))
                self.new_points.append((commit["commit"]["id"], int(newdate), newvalue, commit['commit']['message'].partition('\n')[0]))
            else:
                self.new_points.append((commit["commit"]["id"], int(newdate), newvalue, None))

            curdate = newdate
            curvalue = newvalue


class TSimpleUpdater(IUpdater):
    def __init__(self, signal_name, signal_func, signal_comment_func):
        super(TSimpleUpdater, self).__init__(signal_name, signal_func, signal_comment_func)

    def gen_new_points(self, options):
        value = self.signal_func()
        new_point = None, int(time.time()), value, self.signal_comment_func(value)
        self.new_points.append(new_point)

        if options.verbose > 1:
            print 'TSimpleUpdater: added new point <{}>'.format(new_point)


class EActions(object):
    SHOWNEW = "shownew"
    UPDATE = "update"
    SHOWAT = "showat" # show data at time
    SHOWSINCE = "showsince"
    ALL = [SHOWNEW, UPDATE, SHOWAT, SHOWSINCE]


def get_parser():
    parser = ArgumentParserExt("""Import gencfg db data to clickhouse""")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices = EActions.ALL,
                        help="Obligatory. Action to execute: one of <%s>" % ",".join(EActions.ALL))
    parser.add_argument("-s", "--signals", type=argparse_types.comma_list, default = None,
                        help="Optional. List of signals to process (if not specified process all signals)")
    parser.add_argument('-f', '--updaters-filter', type=argparse_types.pythonlambda, default = lambda x: True,
                        help='Optional. Filter on updaters (e. g. <lambda x: x.signal_name.startswith("sandbox_scheduler_")> to filter only updaters on sandbox schedulers')
    parser.add_argument("-t", "--timestamp", type=int, default=None,
                        help="Optional. Show signal values at specified time (or most close timestamp)")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity (maximum is 2)")
    parser.add_argument('--base-id', type=str, default=EBaseId.GRAPHS_INTERNAL,
                        choices=EBaseId.ALL,
                        help='Optional. Choose clickhouse database: one of <{}>'.format(','.join(EBaseId.ALL)))

    return parser

# ==============================================================
# Value funcs
# ==============================================================

def convert_sandbox_time(s):
    s = s.split('.')[0]
    if s.endswith('Z'):
        s = s[:-1]

    t = time.mktime(time.strptime(s, '%Y-%m-%dT%H:%M:%S'))

    return t


def _task_duration(task):
    starts = task['time']['created']
    ends = task['execution']['finished']

    # something strange: finished task 252603897 does not have finish time
    if ends is None:
        ends = starts

    return convert_sandbox_time(ends) - convert_sandbox_time(starts)


def _success_task_duration(task):
    if task["status"] in ("SUCCESS", "RELEASED"):
        return _task_duration(task)
    return None

def _test_config_generator_duration(task):
    if task.get("build_status", None) == "SUCCESS":
        return _task_duration(task)
    return None

def _interface_commit(commit):
    if "web" in commit.labels:
        return 1
    return None

def _service_commit(commit):
    if "sandbox" in commit.labels:
        return 1
    return None

def _non_interface_commit(commit):
    if (_interface_commit(commit) is not None) or (_service_commit(commit) is not None):
        return None
    return 1

def _kimkim_commit_count(commit):
    if commit.author == 'kimkim' or commit.message.find('commited by kimkim') > 0:
        return 1
    return None

def _machine_count(signal_name):
    """
        Calculate number of machines for specific metaprj
    """

    metaprj = signal_name.partition('_')[0]

    result = 0
    for group in CURDB.groups.get_groups():
        if group.card.master is not None:
            continue
        if group.card.on_update_trigger is not None:
            continue
        if group.card.properties.created_from_portovm_group is not None:
            continue
        if group.card.tags.metaprj != metaprj:
            continue

        result += len(group.getHosts())

    return result

def _assigned_memory(signal_name):
    """
        Calculate memory, assigned to specific metaprj
    """

    metaprj = signal_name.partition('_')[0]

    result = 0
    for group in CURDB.groups.get_groups():
        if group.card.properties.created_from_portovm_group is not None:
            continue
        if group.card.tags.metaprj != metaprj:
            continue
        if group.card.properties.fake_group:
            continue

        if group.card.properties.full_host_group:
            result += sum(map(lambda x: x.memory, group.getHosts()))
        else:
            result += len(group.get_kinda_busy_instances()) * group.card.reqs.instances.memory_guarantee.gigabytes()

    return result


# ========================================= RX-391 START ================================================
def sandbox_scheduler_delay(scheduler_id):
    """Get delay for specified scheduler"""
    lastok = get_scheduler_lastok(scheduler_id)
    if lastok is None:  # could happen if no task finished yet
        lastok = time.time()

    return time.time() - lastok
# ========================================= RX-391 FINISH ===============================================


# =============================================================
# Comment funcs
# =============================================================
def _no_comment(*args):
    return None

UPDATERS = [
    (TSandboxTaskUpdater, "test_config_generator_time", _test_config_generator_duration, _no_comment, "TEST_CONFIG_GENERATOR"),
    (TSandboxTaskUpdater, "build_config_generator_time", _success_task_duration, _no_comment, "BUILD_CONFIG_GENERATOR"),
    (TSandboxTaskUpdater, "release_config_generator_time", _success_task_duration, _no_comment, "RELEASE_CONFIG_GENERATOR"),
    (TSandboxTaskUpdater, "export_to_cauth_time", _success_task_duration, _no_comment, "EXPORT_GENCFG_TO_CAUTH"),
    (TCommitUpdater, "interface_commit_count", _interface_commit, _no_comment),
    (TCommitUpdater, "service_commit_count", _service_commit, _no_comment),
    (TCommitUpdater, "noninterface_commit_count", _non_interface_commit, _no_comment),
    (TCommitUpdater, "kimkim_commit_count", _kimkim_commit_count, _no_comment),
    (TCommitGreenLevelUpdater, "green_time_perc"),
    (TCommitGencfgNotWorking, "gencfg_not_working"),
    (TSimpleUpdater, "addrs_machine_count", functools.partial(_machine_count, "addrs_machine_count"), _no_comment),
    (TSimpleUpdater, "disk_machine_count", functools.partial(_machine_count, "disk_machine_count"), _no_comment),
    (TSimpleUpdater, "freshness_machine_count", functools.partial(_machine_count, "freshness_machine_count"), _no_comment),
    (TSimpleUpdater, "imgs_machine_count", functools.partial(_machine_count, "imgs_machine_count"), _no_comment),
    (TSimpleUpdater, "internal_machine_count", functools.partial(_machine_count, "internal_machine_count"), _no_comment),
    (TSimpleUpdater, "mail_machine_count", functools.partial(_machine_count, "mail_machine_count"), _no_comment),
    (TSimpleUpdater, "maintenance_machine_count", functools.partial(_machine_count, "maintenance_machine_count"), _no_comment),
    (TSimpleUpdater, "music_machine_count", functools.partial(_machine_count, "music_machine_count"), _no_comment),
    (TSimpleUpdater, "news_machine_count", functools.partial(_machine_count, "news_machine_count"), _no_comment),
    (TSimpleUpdater, "reserve_machine_count", functools.partial(_machine_count, "reserve_machine_count"), _no_comment),
    (TSimpleUpdater, "robot_machine_count", functools.partial(_machine_count, "robot_machine_count"), _no_comment),
    (TSimpleUpdater, "saas_machine_count", functools.partial(_machine_count, "saas_machine_count"), _no_comment),
    (TSimpleUpdater, "unknown_machine_count", functools.partial(_machine_count, "unknown_machine_count"), _no_comment),
    (TSimpleUpdater, "unsorted_machine_count", functools.partial(_machine_count, "unsorted_machine_count"), _no_comment),
    (TSimpleUpdater, "video_machine_count", functools.partial(_machine_count, "video_machine_count"), _no_comment),
    (TSimpleUpdater, "web_machine_count", functools.partial(_machine_count, "web_machine_count"), _no_comment),
    (TSimpleUpdater, "addrs_assigned_memory", functools.partial(_assigned_memory, "addrs_assigned_memory"), _no_comment),
    (TSimpleUpdater, "disk_assigned_memory", functools.partial(_assigned_memory, "disk_assigned_memory"), _no_comment),
    (TSimpleUpdater, "freshness_assigned_memory", functools.partial(_assigned_memory, "freshness_assigned_memory"), _no_comment),
    (TSimpleUpdater, "imgs_assigned_memory", functools.partial(_assigned_memory, "imgs_assigned_memory"), _no_comment),
    (TSimpleUpdater, "internal_assigned_memory", functools.partial(_assigned_memory, "internal_assigned_memory"), _no_comment),
    (TSimpleUpdater, "mail_assigned_memory", functools.partial(_assigned_memory, "mail_assigned_memory"), _no_comment),
    (TSimpleUpdater, "maintenance_assigned_memory", functools.partial(_assigned_memory, "maintenance_assigned_memory"), _no_comment),
    (TSimpleUpdater, "music_assigned_memory", functools.partial(_assigned_memory, "music_assigned_memory"), _no_comment),
    (TSimpleUpdater, "news_assigned_memory", functools.partial(_assigned_memory, "news_assigned_memory"), _no_comment),
    (TSimpleUpdater, "reserve_assigned_memory", functools.partial(_assigned_memory, "reserve_assigned_memory"), _no_comment),
    (TSimpleUpdater, "robot_assigned_memory", functools.partial(_assigned_memory, "robot_assigned_memory"), _no_comment),
    (TSimpleUpdater, "saas_assigned_memory", functools.partial(_assigned_memory, "saas_assigned_memory"), _no_comment),
    (TSimpleUpdater, "unknown_assigned_memory", functools.partial(_assigned_memory, "unknown_assigned_memory"), _no_comment),
    (TSimpleUpdater, "unsorted_assigned_memory", functools.partial(_assigned_memory, "unsorted_assigned_memory"), _no_comment),
    (TSimpleUpdater, "video_assigned_memory", functools.partial(_assigned_memory, "video_assigned_memory"), _no_comment),
    (TSimpleUpdater, "web_assigned_memory", functools.partial(_assigned_memory, "web_assigned_memory"), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_9963", functools.partial(sandbox_scheduler_delay, 9963), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_9499", functools.partial(sandbox_scheduler_delay, 9499), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_9498", functools.partial(sandbox_scheduler_delay, 9498), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_8779", functools.partial(sandbox_scheduler_delay, 8779), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_8573", functools.partial(sandbox_scheduler_delay, 8573), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_8295", functools.partial(sandbox_scheduler_delay, 8295), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_7991", functools.partial(sandbox_scheduler_delay, 7991), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_7335", functools.partial(sandbox_scheduler_delay, 7335), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_6104", functools.partial(sandbox_scheduler_delay, 6104), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_6096", functools.partial(sandbox_scheduler_delay, 6096), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_4227", functools.partial(sandbox_scheduler_delay, 4227), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_2504", functools.partial(sandbox_scheduler_delay, 2504), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_2489", functools.partial(sandbox_scheduler_delay, 2489), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_2074", functools.partial(sandbox_scheduler_delay, 2074), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_1919", functools.partial(sandbox_scheduler_delay, 1919), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_1791", functools.partial(sandbox_scheduler_delay, 1791), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_1512", functools.partial(sandbox_scheduler_delay, 1512), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_1307", functools.partial(sandbox_scheduler_delay, 1307), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_981", functools.partial(sandbox_scheduler_delay, 981), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_932", functools.partial(sandbox_scheduler_delay, 932), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_931", functools.partial(sandbox_scheduler_delay, 931), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_927", functools.partial(sandbox_scheduler_delay, 927), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_926", functools.partial(sandbox_scheduler_delay, 926), _no_comment),
    (TSimpleUpdater, "sandbox_scheduler_924", functools.partial(sandbox_scheduler_delay, 924), _no_comment),
]

def jsmain(d):
    options = get_parser().parse_json(d)

    return main(options)

def main(options):
    if options.action in (EActions.SHOWNEW, EActions.UPDATE):
        updaters = []
        for updater_data in UPDATERS:
            updater_class = updater_data[0]
            updater_params = updater_data[1:]
            updaters.append(updater_class(*updater_params))

        updaters = [x for x in updaters if options.updaters_filter(x)]

        for updater in updaters:
            updater.gen_new_points(options)

        if options.action == EActions.SHOWNEW:
            for updater in updaters:
                updater.show_new_points()
        elif options.action == EActions.UPDATE:
            for updater in updaters:
                updater.add_new_points()
        else:
            raise Exception("Unknown action <%s>" % options.action)
    elif options.action == EActions.SHOWAT:
        if options.timestamp is None:
            options.timestamp = int(time.time())

        if options.verbose > 0:
            print "Time %s:" % time.strftime("%Y-%m-%d %H:%M", time.localtime(options.timestamp))

        result = {}
        for updater_info in UPDATERS:
            signal_name = updater_info[1]
            if options.signals is not None and signal_name not in options.signals:
                continue

            clickhouse_result = run_clickhouse_query(("SELECT value FROM kpi_graphs_data WHERE signal_name == '{}' AND timestamp < {}"
                                                      " ORDER BY timestamp DESC LIMIT 1").format(signal_name, options.timestamp), base_id=options.base_id)
            if clickhouse_result:
                value = float(clickhouse_result[0][0])
            else:
                value = 0

            result[signal_name] = value
            if options.verbose > 0:
                print "    %s: %s" % (signal_name, value)
        return result
    elif options.action == EActions.SHOWSINCE:
        if options.verbose > 0:
            print "Time %s:" % time.strftime("%Y-%m-%d %H:%M", time.localtime(options.timestamp))

        conn = get_db_connection()
        result = defaultdict(list)

        for updater_info in UPDATERS:
            signal_name = updater_info[1]
            if options.signals is not None and signal_name not in options.signals:
                continue

            if options.verbose > 0:
                print '   {}:'.format(signal_name)

            clickhouse_result = run_clickhouse_query(("SELECT timestamp, value FROM kpi_graphs_data WHERE signal_name == '{}'"
                                                      " AND timestamp >= {} ORDER BY timestamp ASC").format(signal_name, options.timestamp), base_id=options.base_id)
            for ts, v in clickhouse_result:
                ts = int(ts)
                v = float(v)
                result[signal_name].append((ts, v))
                if options.verbose > 0:
                    print '        ts {}, value {}'.format(ts, v)

        return result
    else:
        raise Exception("Unknown action <%s>" % options.action)

if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)
