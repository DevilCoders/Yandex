# -*- coding: utf-8 -*-

import random

from django.utils import timezone
from django.db.models import Q, Count

from core.settings import ZERO_TIME
from core.models import Estimation, TaskPack, TaskPool, Task, User
from core.actions import qconstructor, rawsql, timeprocessor


def create_estimation(user, tasks, taskpack=None):
    cur_time = taskpack.last_update if taskpack else timezone.now()
    e_list = [Estimation(
        task=t, user=user, create_time=cur_time,
        start_time=ZERO_TIME, complete_time=ZERO_TIME,
        status=Estimation.Status.ASSIGNED,
        json_value={},
        taskpack=taskpack) for t in tasks]
    if len(e_list) == 1:
        e_list[0].save()
    else:
        Estimation.objects.bulk_create(e_list)
    return e_list


def get_t_testing(t_done, t_verified):
    K = 0.05
    t_testing = int(
        (t_done * (2.0 + 10000.0 / (t_done + 100.0)) / 100.0) *
        (1.0 + K) -
        t_verified
    )
    return max(0, t_testing)


def get_inspection_tasks_stat(user, taskpool):
    # we assume assessor solves all packs he took in this period
    # and current pack
    ang_period_start = timeprocessor.ang_period_start()
    done_tasks_count = Estimation.objects.filter(
        user=user,
        status__in=(Estimation.Status.COMPLETE,
                    Estimation.Status.ASSIGNED,
                    Estimation.Status.SKIPPED),
        create_time__gte=ang_period_start,
    ).count() + taskpool.pack_size

    verified_tasks_count = Estimation.objects.filter(
        user=user,
        status__in=(Estimation.Status.COMPLETE,
                    Estimation.Status.ASSIGNED,
                    Estimation.Status.SKIPPED),
        create_time__gte=ang_period_start,
    ).exclude(
        task__status=Task.Status.REGULAR
    ).count()

    needed_inspection_tasks_count = get_t_testing(done_tasks_count,
                                                  verified_tasks_count)

    available_inspection_tasks_count = Task.objects.filter(
        taskpool_id=taskpool.pk,
        status=Task.Status.INSPECTION
    ).count()

    return (available_inspection_tasks_count, needed_inspection_tasks_count)


# takes tasks for assessor and creates pack
# returns pack.pk on success, None on fail
def create_taskpack(user, taskpool):
    (available_inspection_tasks_count, needed_inspection_tasks_count) =\
        get_inspection_tasks_stat(user, taskpool)
    max_size_inspection_pack = int(taskpool.pack_size * 0.25)
    needed_inspection_tasks_count = min(needed_inspection_tasks_count,
                                        max_size_inspection_pack)
    reg2insp_task_pks = []
    if available_inspection_tasks_count < needed_inspection_tasks_count:
        # change regular tasks to inspection
        reg2insp_task_pks = Task.objects.filter(
            taskpool_id=taskpool.pk,
            status=Task.Status.REGULAR
        ).values_list('pk', flat=True).order_by('?')[
            :(needed_inspection_tasks_count -
              available_inspection_tasks_count)]
        change_task_qs_status(reg2insp_task_pks, Task.Status.INSPECTION)

    inspection_tasks = list(
        rawsql.get_inspection_tasks_for_estimations(
            taskpool, needed_inspection_tasks_count
        )
    )
    if taskpool.pack_size > len(inspection_tasks):
        regular_tasks = list(
            rawsql.get_tasks_for_estimations(taskpool, inspection_tasks)
        )
    else:
        regular_tasks = []
    tasks = inspection_tasks + regular_tasks
    if len(tasks) < taskpool.pack_size:
        change_task_qs_status(reg2insp_task_pks, Task.Status.REGULAR)
        return None
    random.shuffle(tasks)
    new_pack = TaskPack(taskpool=taskpool, user=user,
                        last_update=timezone.now(),
                        status=TaskPack.Status.ACTIVE)
    new_pack.save()
    create_estimation(user, tasks, new_pack)
    return new_pack.pk


def change_task_qs_status(task_pks_qs, to_status):
    # Hack for: "This version of MySQL doesn't yet support
    # 'LIMIT & IN/ALL/ANY/SOME subquery'"
    # This builds a list of "OR id EQUAL TO i"
    if task_pks_qs:
        Task.objects.filter(reduce(
            lambda x, y: x | y, (Q(id=i) for i in task_pks_qs)
        )).update(
            status=to_status
        )


def take_aadmin_task_batch(user, task_ids):
    tasks_query = Task.objects.select_related(
        'taskpool'
    ).filter(
        pk__in=task_ids,
        status=Task.Status.INSPECTION,
        taskpool__status=TaskPool.Status.FINISHED
    ).exclude(
        estimation__user=user,
        estimation__status__in=(Estimation.Status.REJECTED,
                                Estimation.Status.ASSIGNED,
                                Estimation.Status.COMPLETE,
                                Estimation.Status.SKIPPED,)
    )
    return taken_tasks(tasks_query, user)


def assign_task_batch(user, taskpool_id, task_ids):
    exclude_kwargs = {}
    # AN and DV can take disabled ests
    if user.role == User.Role.AADMIN:
        exclude_kwargs = {'taskpool__status': TaskPool.Status.DISABLED}
    tasks_query = Task.objects.select_related(
        'taskpool'
    ).filter(
        pk__in=task_ids,
        taskpool_id=taskpool_id
    ).exclude(
        **exclude_kwargs
    )
    return taken_tasks(tasks_query, user)


def taken_tasks(tasks_query, user):
    tasks = list(tasks_query)
    aa_ests = []
    aa_ests_ext = []
    if tasks:
        aa_ests_query = Estimation.objects.filter(task_id__in=tasks,
                                                  user=user)
        cur_time = timezone.now()
        # No more than 1 est on each task for not AS exist
        aa_ests_query.exclude(status__in=(Estimation.Status.COMPLETE,
                                          Estimation.Status.ASSIGNED,
                                          Estimation.Status.SKIPPED)).update(
            create_time=cur_time,
            start_time=ZERO_TIME,
            rendering_time=ZERO_TIME,
            r_time=ZERO_TIME,
            c_time=ZERO_TIME,
            i_time=ZERO_TIME,
            complete_time=ZERO_TIME,
            status=Estimation.Status.ASSIGNED,
            json_value=Estimation.DEFAULT_VALUE,
        )

        aa_ests = list(aa_ests_query)
        aa_ests_task_ids = [est.task_id for est in aa_ests]

        tasks_ext = [task for task in tasks
                     if task.id not in aa_ests_task_ids]
        aa_ests_ext = create_estimation(user, tasks_ext)
    return (tasks, aa_ests + aa_ests_ext)


def assign_task_by_assessor(aadmin, assessor_id):
    task = list(Task.objects.filter(
        qconstructor.complete_est_q(prefix='estimation__'),
        status=Task.Status.REGULAR,
        taskpool__status=TaskPool.Status.FINISHED,
    ).annotate(
        completed=Count('estimation')
    ).filter(
        estimation__user_id=assessor_id,
    ).exclude(
        estimation__user_id=aadmin.pk,
    ).order_by(
        'completed'
    )[:1])
    if not task:
        return None
    task = task[0]
    task.status = Task.Status.INSPECTION
    task.save()
    return take_aadmin_task_batch(aadmin, (task.pk,))
