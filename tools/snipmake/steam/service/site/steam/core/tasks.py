# -*- coding: utf-8 -*-

import json
import os
import tempfile

from calendar import mdays
from celery import task
from datetime import timedelta
from django.db import transaction
from django.db.models import Q, Count
from django.utils import timezone
from django.utils.translation import ugettext_noop as _

from core.settings import (ASSIGNED_EST_TIMEOUT, SKIPPED_EST_TIMEOUT,
                           TEMP_FILE_TIMEOUT, TEMP_ROOT)
from core.actions import (ang, mailsender, qconstructor,
                          rawsql, statcollector, timeprocessor)
from core.actions.storage import Storage
from core.actions.steambgtask import SteamTask
from core.hard.loghandlers import SteamLogger
from core.models import (ANGReport, Correction, Estimation, Notification,
                         SnippetPool, Syncronizer, Task, TaskPack,
                         TaskPool, User)


@task(name='core.tasks.timeout_estimations')
@transaction.commit_on_success
def timeout_estimations():
    Estimation.objects.filter(
        Q(
            create_time__lte=timezone.now() - ASSIGNED_EST_TIMEOUT,
            status=Estimation.Status.ASSIGNED
        ) | Q(
            create_time__lte=timezone.now() - SKIPPED_EST_TIMEOUT,
            status=Estimation.Status.SKIPPED
        ),
        taskpack_id__isnull=True
    ).exclude(
        task__taskpool__status=TaskPool.Status.DISABLED
    ).update(
        status=Estimation.Status.TIMEOUT,
        json_value=Estimation.DEFAULT_VALUE
    )


@task(name='core.tasks.temp_root_cleanup')
def temp_root_cleanup():
    for filename in os.listdir(TEMP_ROOT):
        filename = os.path.join(TEMP_ROOT, filename)
        if (
            timezone.datetime.fromtimestamp(os.path.getmtime(filename),
                                            timezone.get_default_timezone()) <
            timezone.now() - TEMP_FILE_TIMEOUT
        ):
            try:
                os.remove(filename)
            except OSError:
                pass


# task decorator must be first
@task(name='core.tasks.send_stats')
@transaction.commit_on_success
def send_stats():
    celery_id = str(send_stats.request.id)
    correction_time = timezone.now()
    (correction_ids, revision_numbers, stats) = statcollector.get_stats()
    if stats == '':
        SteamLogger.info('No changes since previous report. Nothing to send.',
                         type='ANG_REPORT_EVENT')
        return
    handle, tmp_name = tempfile.mkstemp(dir=TEMP_ROOT)
    os.write(handle, stats)
    os.close(handle)
    storage_id = Storage.store_raw_file(tmp_name)
    if storage_id:
        os.remove(tmp_name)
    else:
        SteamLogger.warning('Could not store ANG report from %(tmp_name)s',
                            type='ANG_REPORT_WARNING', tmp_name=tmp_name)
    batch_time = timezone.now()
    reports = []
    taskpool_ids = []
    status = ang.send_report(stats)
    for taskpool_id, revision_number in revision_numbers:
        reports.append(ANGReport(celery_id=celery_id, time=batch_time,
                                 taskpool_id=taskpool_id,
                                 revision_number=revision_number,
                                 status=status,
                                 storage_id=storage_id))
        taskpool_ids.append(taskpool_id)
    ANGReport.objects.bulk_create(reports)
    # we should not delete corrections later than
    # time of data collecting (they can be created by other transaction)
    correction_cleanup_qs = Correction.objects.filter(
        assessor_est__task__taskpool_id__in=taskpool_ids
    ).exclude(
        Q(pk__in=correction_ids) |
        Q(time__gt=correction_time)
    )
    if status == ANGReport.Status.ACKNOWLEDGED:
        Correction.objects.filter(
            assessor_est__task__taskpool_id__in=taskpool_ids,
            assessor_est__task__status=Task.Status.COMPLETE,
            pk__in=correction_ids, status=Correction.Status.ACTUAL
        ).update(status=Correction.Status.REPORTED)
    else:
        correction_cleanup_qs = correction_cleanup_qs.exclude(
            status=Correction.Status.REPORTED
        )
    correction_cleanup_qs.delete()


@task(name='core.tasks.finish_taskpools')
@transaction.commit_on_success
def finish_taskpools():
    # finishing by deadline
    cur_date = timezone.localtime(timezone.now()).date()
    TaskPack.objects.filter(
        status=TaskPack.Status.ACTIVE,
        taskpool__deadline__lte=cur_date,
    ).exclude(
        # ACTIVE packs in FINISHED taskpools should be timeouted
        # so we ignore only disabled taskpools
        taskpool__status=TaskPool.Status.DISABLED
    ).update(
        status=TaskPack.Status.TIMEOUT
    )
    Notification.objects.filter(
        kind=Notification.Kind.DEADLINE,
        taskpack__status=TaskPack.Status.TIMEOUT
    ).delete()
    tps_qs = TaskPool.objects.filter(
        status__in=(TaskPool.Status.ACTIVE,
                    TaskPool.Status.OVERLAPPED),
        deadline__lte=cur_date
    )
    for tp in tps_qs:
        if tp.status == TaskPool.Status.ACTIVE:
            ang.reset_taskpool(tp, 'finish')
    # overlapping
    tps_qs = TaskPool.objects.filter(
        status=TaskPool.Status.ACTIVE
    ).exclude(
        pk__in=rawsql.taskpools_not_to_finish()
    )
    for tp in tps_qs:
        ang.reset_taskpool(tp, 'overlap')
    # finishing by overlap
    TaskPool.objects.filter(
        status=TaskPool.Status.OVERLAPPED
    ).exclude(
        taskpack__status=TaskPack.Status.ACTIVE
    ).update(
        status=TaskPool.Status.FINISHED
    )


@task(name='core.tasks.notify_assessors')
@transaction.commit_on_success
def notify_assessors():
    tomorrow = timezone.localtime(timezone.now()).date() + timedelta(1)
    taskpacks_info = TaskPack.objects.select_for_update().annotate(
        notifications=Count('notification')
    ).filter(
        status=TaskPack.Status.ACTIVE,
        taskpool__deadline__lte=tomorrow,
    ).values(
        'pk',
        'user_id',
        'user__login',
        'user__language',
        'emailed',
        'taskpool_id',
        'notifications'
    )
    new_notifications = [Notification(kind=Notification.Kind.DEADLINE,
                                      user_id=info['user_id'],
                                      taskpack_id=info['pk'])
                         for info in taskpacks_info
                         if info['notifications'] == 0]
    Notification.objects.bulk_create(new_notifications)
    newly_emailed = []
    aggregated_infos = {}
    for info in taskpacks_info:
        if not info['emailed']:
            taskpool_id = info['taskpool_id']
            aggregated_infos[taskpool_id] = aggregated_infos.get(
                taskpool_id, []
            ) + [info]
    for taskpool_id in aggregated_infos:
        as_logins = [info['user__login']
                     for info in aggregated_infos[taskpool_id]]
        # users can be from multiple countries,
        # but if they solve tasks from same taskpool,
        # they must speak one language
        country = aggregated_infos[taskpool_id][0]['user_language']
        if mailsender.notify_about_deadline(as_logins, country, taskpool_id):
            newly_emailed += [info['pk']
                              for info in aggregated_infos[taskpool_id]]
    TaskPack.objects.filter(
        pk__in=newly_emailed
    ).update(
        emailed=True
    )


@task(name='core.tasks.email_aadmins')
@transaction.commit_on_success
def email_aadmins():
    end_date = timezone.localtime(timezone.now()).date()
    prev_month = (end_date - timedelta(end_date.day)).month
    start_date = end_date - timedelta(mdays[prev_month])
    try:
        syncronizer = Syncronizer.objects.select_for_update().all()[0]
    except IndexError:
        syncronizer = Syncronizer(aadmin_email=start_date)
        syncronizer.save()
    if syncronizer.aadmin_email > start_date:
        SteamLogger.info(
            'Nothing to send in monthly statictics for aadmins',
            type='AADMIN_EMAIL_EVENT',
        )
        return
    (start_date, end_date) = timeprocessor.date_range(start_date, end_date)
    est_infos = Estimation.objects.filter(
        qconstructor.complete_est_q(),
        taskpack_id__isnull=False,
        complete_time__gte=start_date,
        complete_time__lt=end_date
    ).values(
        'json_value',
        'user__login',
        'user__language'
    )
    aggregated_infos = {}
    for est_info in est_infos:
        if not aggregated_infos.get(est_info['user__language']):
            aggregated_infos[est_info['user__language']] = {}
        if not aggregated_infos[est_info['user__language']].get(
            est_info['user__login']
        ):
            aggregated_infos[est_info['user__language']][
                est_info['user__login']
            ] = {'total': 0, 'linked': 0}
        aggregated_infos[est_info['user__language']][
            est_info['user__login']
        ]['total'] += 1
        try:
            if json.loads(est_info['json_value'],
                          encoding='utf-8').get('linked'):
                aggregated_infos[est_info['user__language']][
                    est_info['user__login']
                ]['linked'] += 1
        except (TypeError, ValueError):
            continue
    aadmins = User.objects.filter(
        role=User.Role.AADMIN
    ).values(
        'login',
        'language'
    )
    aggregated_aadmins = {}
    for aadmin in aadmins:
        if not aggregated_aadmins.get(aadmin['language']):
            aggregated_aadmins[aadmin['language']] = []
        aggregated_aadmins[aadmin['language']].append(aadmin['login'])
    for language in aggregated_aadmins:
        infos = aggregated_infos.get(language)
        if not infos:
            continue
        if mailsender.send_stats_to_aadmins(aggregated_aadmins[language],
                                            infos, language):
            SteamLogger.info(
                'Sent monthly statictics to %(count)d assessor admins in %(country)s country',
                type='AADMIN_EMAIL_SUCCESS',
                country=language, count=len(aggregated_aadmins[language])
            )
        else:
            SteamLogger.error(
                'Failed to send monthly statictics to %(count)d assessor admins in %(country)s country',
                type='AADMIN_EMAIL_ERROR',
                country=language, count=len(aggregated_aadmins[language])
            )
    syncronizer.aadmin_email = end_date
    syncronizer.save()


@task(name='core.tasks.bg_download_serp', base=SteamTask)
def bg_download_serp(downloader, title, default_tpl):
    uid = str(bg_download_serp.request.id)
    downloader.switch_uid(uid)
    bg_task = bg_download_serp.bg_task
    bg_task.status = _('DOWNLOADING')
    bg_task.save()
    handle, tmp_name = tempfile.mkstemp(dir=TEMP_ROOT,
                                        prefix=''.join((uid, '.')))
    os.close(handle)
    if downloader.download(tmp_name):
        bg_task.status = _('STORING')
        bg_task.save()
        storage_id = Storage.store_serp(tmp_name)
        if storage_id:
            with transaction.commit_on_success():
                bg_task.status = _('SUCCESS')
                bg_task.storage_id = storage_id
                bg_task.save()
                save_snippetpool(storage_id, tmp_name, title, default_tpl,
                                 bg_task.user)
            return
    bg_task.status = _('ERROR')
    bg_task.save()


@task(name='core.tasks.bg_store_snippetpool', base=SteamTask)
def bg_store_snippetpool(tmp_name, title, default_tpl, json_pool):
    bg_task = bg_store_snippetpool.bg_task
    bg_task.status = _('STORING')
    bg_task.save()
    storage_id = Storage.store_snip_pool(tmp_name, json_pool, default_tpl == TaskPool.TypePool.RCA)
    if storage_id:
        with transaction.commit_on_success():
            bg_task.status = _('SUCCESS')
            bg_task.storage_id = storage_id
            bg_task.save()
            save_snippetpool(storage_id, tmp_name, title, default_tpl,
                             bg_task.user)
        return
    bg_task.status = _('ERROR')
    bg_task.save()


def save_snippetpool(storage_id, tmp_name, title, default_tpl, user):
    iterator = Storage.snippet_iterator(storage_id)
    snippet_pool = SnippetPool(user=user, title=title,
                               tpl=default_tpl,
                               upload_time=timezone.now(),
                               count=iterator.count(),
                               storage_id=storage_id)
    snippet_pool.save()
    os.remove(tmp_name)
