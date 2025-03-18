# -*- coding: utf-8 -*-

from django.db.models import F

from core.models import (ANGReport, Correction, Estimation,
                         Task, TaskPack, TaskPool, User)
from core.settings import ZERO_TIME


# returns dict
# { user.login: [count, error, verified, rejected, closed, ticks] }
def user_stats(taskpool_id):
    ests = Estimation.objects.filter(
        task__taskpool_id=taskpool_id,
        status__in=(Estimation.Status.COMPLETE, Estimation.Status.REJECTED)
    ).select_related(
        'user',
        'task',
        'taskpack',
        'correction'
    )
    # SAVED corrections are not deleted while stats collection
    # we shall delete them when aadmin checks task and overwrites them
    correction_ids = list(Correction.objects.filter(
        status=Correction.Status.SAVED,
        time__gt=F('aadmin_est__complete_time')
    ).values_list('id', flat=True))
    stats = {}
    for est in ests:
        if est.status == Estimation.Status.COMPLETE:
            # we should not delete latest actual correction
            if (
                est.task.status == Task.Status.COMPLETE and
                est.is_visible()
            ):
                correction_ids.append(est.correction_id)
        if (
            est.user.role != User.Role.ASSESSOR or
            est.taskpack.status != TaskPack.Status.FINISHED
        ):
            continue
        if not stats.get(est.user.login):
            stats[est.user.login] = [0, 0, 0, 0, 0, 0]
        if est.status == Estimation.Status.COMPLETE:
            stats[est.user.login][4] += 1
            stats[est.user.login][0] += 1
            stats[est.user.login][5] += int((est.complete_time -
                                             est.start_time).total_seconds())
            if est.task.status == Task.Status.COMPLETE:
                stats[est.user.login][2] += 1
                if est.correction.errors:
                    stats[est.user.login][1] += 1
        # else est.status == Estimation.Status.REJECTED
        else:
            stats[est.user.login][3] += 1
    return (correction_ids, stats)


def get_changed_taskpool_ids(time):
    new_pack_taskpools = TaskPack.objects.filter(
        last_update__gt=time,
        status=TaskPack.Status.FINISHED
    ).exclude(
        taskpool__status=TaskPool.Status.DISABLED
    ).distinct().values_list('taskpool_id', flat=True)
    new_corr_taskpools = Correction.objects.filter(
        status=Correction.Status.ACTUAL,
        time__gt=time
    ).exclude(
        assessor_est__task__taskpool__status=TaskPool.Status.DISABLED
    ).distinct().values_list('assessor_est__task__taskpool_id', flat=True)
    return set(new_pack_taskpools).union(set(new_corr_taskpools))


def get_stats():
    stats = ''
    revision_numbers = []
    correction_ids = []
    # select for update for strictly successive database transactions
    latest_report = ANGReport.objects.select_for_update().filter(
        status=ANGReport.Status.ACKNOWLEDGED
    ).order_by('-time')[:1]
    if latest_report:
        latest_report_time = latest_report[0].time
    else:
        latest_report_time = ZERO_TIME
    taskpool_ids = get_changed_taskpool_ids(latest_report_time)
    taskpools = TaskPool.objects.filter(
        pk__in=taskpool_ids
    )
    for taskpool in taskpools:
        acknowledged_reports = taskpool.angreport_set.filter(
            status=ANGReport.Status.ACKNOWLEDGED
        ).order_by('-time')[:1]
        if acknowledged_reports:
            revision_number = acknowledged_reports[0].revision_number + 1
        else:
            revision_number = 1
        revision_numbers.append((taskpool.id, revision_number))
        (taskpool_correction_ids,
         taskpool_user_stats) = user_stats(taskpool.id)
        correction_ids += taskpool_correction_ids
        for login in taskpool_user_stats:
            counters = '\t'.join([str(counter)
                                  for counter in taskpool_user_stats[login]])
            stat_line = '%s\t%s\t%d\t%s\t%s' % (login, taskpool.ang_taskset_id,
                                            revision_number, counters, 'true')
            stats = '\n'.join((stats, stat_line))
    return (correction_ids, revision_numbers, stats.lstrip())
