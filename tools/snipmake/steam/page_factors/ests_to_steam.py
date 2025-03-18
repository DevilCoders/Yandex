#!/usr/bin/python
import json
import sys

from django.utils import timezone
try:
    from core.models import Task, TaskPool, Estimation
except ImportError:
    print '''
set the following environment variables to run ests_to_steam:

export PYTHONPATH=$PYTHONPATH:$ARCADIA_ROOT/tools/snipmake/steam/service/site/steam
export DJANGO_SETTINGS_MODULE=ui.settings
'''
    exit(1)

time = timezone.now()

MIN_TASK_ID = 0
MAX_TASK_ID = 1
USER_ID = '1120000000019838'
ESTS_FILE = 'estimations2.txt'


def usage():
    print('Usage: ests_to_steam.py {store|load} [-f ests_file] [min_task_id [max_task_id]]')


def load():
    human_file = open('human_ests.tsv', 'w')
    robot_file = open('robot_ests.tsv', 'w')
    ests = Estimation.objects.filter(
        task__taskpool__kind=TaskPool.Type.PAGE_SEGMENTATION,
        status=Estimation.Status.COMPLETE
    ).extra(
        select={'docid':
                'CAST(core_task.request AS UNSIGNED)'},
        order_by=('docid', 'user__login')
    ).values(
        'user__login',
        'docid',
        'json_value'
    )
    for est in ests:
        line = '\t'.join((str(est['docid']), est['user__login'], est['json_value']))
        if est['user__login'] == 'robot-steam-as':
            outf = robot_file
        else:
            outf = human_file
        print >> outf, line
    robot_file.close()
    human_file.close()


def store():
    tasks = open(ESTS_FILE, 'r')
    prev_reqs = set()
    for i, line in enumerate(tasks):
        req = line.split('\t', 1)[0]
        if i < MIN_TASK_ID:
            prev_reqs.add(req)
            continue
        elif i >= MAX_TASK_ID:
            break
        if req in prev_reqs:
            continue
        try:
            est = open('ests/estimations.%d.res.txt' % i, 'r')
        except IOError:
            continue
        else:
            prev_reqs.add(req)
            value = est.read().strip().split('\t')[2]
            est.close()
        try:
            value = json.loads(value)
            t = Task.objects.get(request=req, taskpool__kind='SGM')
            e = Estimation(task_id=t.id, user_id=USER_ID, create_time=time,
                        start_time=time, complete_time=time, status='C',
                        json_value=value)
            e.save()
        except Exception as ex:
            print('line %d: %s' % (i, ex))

    tasks.close()


if __name__ == '__main__':
    try:
        action = sys.argv[1]
    except IndexError:
        usage()
        exit(1)
    idx = 2
    while idx < len(sys.argv):
        if sys.argv[idx] == '-f':
            try:
                ESTS_FILE = sys.argv[idx + 1]
                idx += 2
            except IndexError:
                idx += 1
        else:
            try:
                MIN_TASK_ID = int(sys.argv[idx])
                idx += 1
                MAX_TASK_ID = int(sys.argv[idx])
            except (ValueError, IndexError):
                pass
            idx += 1
    if action == 'store':
        store()
        exit(0)
    if action == 'load':
        load()
        exit(0)
    usage()
    exit(1)
