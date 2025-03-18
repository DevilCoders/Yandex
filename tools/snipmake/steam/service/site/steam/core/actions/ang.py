# -*- coding:utf-8 -*-

import urllib2
import json
import requests
from django.utils import timezone

from core.hard.loghandlers import SteamLogger
from core.models import ANGReport, Estimation, Notification, TaskPack, TaskPool
from core.settings import ZERO_TIME
from ui.settings import ANG_FRONT

TASKSET_TYPE = 'STEAM_ESTIM'
TASKSET_TYPE_RCA = 'STEAM_ESTIM_RCA'


def ask_ang(url, error_type):
    url = ''.join((ANG_FRONT, url))
    try:
        response = urllib2.urlopen(url)
    except urllib2.HTTPError as err:
        SteamLogger.error('HTTP error %(HTTP_code)d: %(HTTP_msg)s url:%(req)s',
                          HTTP_code=err.code, HTTP_msg=err.msg,
                          req=url, type=error_type)
        return None
    except Exception as exc:
        SteamLogger.warning('Could not get response for %(req)s:%(error)s',
                            req=url, type=error_type, error=exc)
        return None

    try:
        ans = json.loads(response.read(), encoding='utf-8')
    except Exception as exc:
        SteamLogger.warning('%(error)s from %(req)s',
                            type=error_type, error=exc, req=url)
        return None

    if 'error' in ans:
        SteamLogger.error('%(error)s', type=error_type, error=ans['error'])
        return None
    return ans


def json_val(obj, path, error_type):
    n = obj
    try:
        for v in path:
            n = n[v]
        return n
    except KeyError as ke:
        SteamLogger.error('Key %(error)s not found in %(json)s',
                          type=error_type, error=ke, json=obj)
    return None


def get_user_info(user):
    error_type = 'ANG_USER_INFO_ERROR'
    url = '/external/api/user/info?user-login=%s&task-type=%s&task-type=%s' % (
        user.login, TASKSET_TYPE, TASKSET_TYPE_RCA
    )
    ans = ask_ang(url, error_type)
    return ans


def available_taskset_ids(user):
    ans = get_user_info(user)
    if ans:
        td = json_val(ans, ('task-type-data',), 'ANG_USER_TASKSET_ERROR')
        if td:
            return reduce(lambda res, x: res + (
                json_val(x, ('active',), 'ANG_USER_TASKSET_ERROR') or []
            ), td, [])
    SteamLogger.warning('No ANG taskset_ids available for %(login)s',
                        type='ANG_USER_WARNING', login=user.login)
    return []


def get_user_status(request):
    ang_session_value = 'ang_status'
    if request.session.get(ang_session_value, None):
        return True
    ans = get_user_info(request.yauser.core_user)
    if ans and json_val(ans, ('user', 'active'),
                        'ANG_USER_STATUS_ERROR') is True:
        request.session[ang_session_value] = True
        SteamLogger.info('Ang user %(login)s accepted', type='ANG_USER_EVENT',
                         login=request.yauser.core_user.login)
        return True
    return False


def create_taskpool(taskpool):
    error_type = 'ANG_RUN_TASKPOOL_ERROR'
    url = make_creation_url(taskpool)
    ans = ask_ang(url, error_type)
    if ans:
        taskset_id = json_val(ans, ('task-set-id',), error_type)
        if taskset_id:
            taskpool.ang_taskset_id = taskset_id
            taskpool.save()
            SteamLogger.info('Start taskpool %(tp_id)d: %(tp_name)s with task_set_id %(ts_id)s',
                             type='ANG_TASKPOOL_EVENT', tp_id=taskpool.id,
                             tp_name=taskpool.title, ts_id=taskset_id)
            return taskset_id
    return None


def make_creation_url(taskpool):
    countries = taskpool.tpcountry_set.all()
    main_country = countries[0].for_ang()
    add_countries = ''.join([
        '&add-regional-type=%s' % country.for_ang()
        for country in countries[1:]
    ])
    return '/external/api/task/set/create?task-type=%s&count=%d&regional-type=%s%s&comment=%s' % (
        get_taskset_type(taskpool), taskpool.overlap_count(),
        main_country, add_countries, 'STEAM'
    )


def get_taskset_type(taskpool):
    if taskpool.kind_pool == TaskPool.TypePool.MULTI_CRITERIAL:
        return TASKSET_TYPE
    if taskpool.kind_pool == TaskPool.TypePool.RCA:
        return TASKSET_TYPE_RCA
    raise ValidationError("Empty or unknown type pool " + taskpool.kind_pool)


def reset_taskpool(taskpool, control):
    error_type = ''.join(['ANG_TASKPOOL_', control.upper()])
    cur_date = timezone.localtime(timezone.now()).date()
    if control == 'start':
        if taskpool.status == TaskPool.Status.ACTIVE:
            return True
        if taskpool.status in (TaskPool.Status.FINISHED,
                               TaskPool.Status.OVERLAPPED):
            return False
        if not taskpool.ang_taskset_id:
            if create_taskpool(taskpool):
                action = None
            else:
                return False
        else:
            action = 'open'
        if taskpool.deadline <= cur_date:
            action = None
    else:
        action = 'close'
        if control == 'overlap':
            if taskpool.status == TaskPool.Status.OVERLAPPED:
                return True
            elif taskpool.status != TaskPool.Status.ACTIVE:
                return False
        else:
            # control in ('disable', 'finish')
            if taskpool.status in (TaskPool.Status.DISABLED,
                                   TaskPool.Status.FINISHED):
                return True
            elif taskpool.status == TaskPool.Status.OVERLAPPED:
                action = None

    if action:
        url = '/external/api/task/set?task-set-id=%s&action=%s' % (
            taskpool.ang_taskset_id, action
        )
        ans = ask_ang(url, error_type)
        if not (ans and json_val(ans, ('response',), error_type) == 'OK'):
            return False

    if control == 'start':
        # TaskPool activation for assessors
        # we must re-create ASSIGNED and SKIPPED estimations
        # and re-enable packs
        recreate_estimations(taskpool.id)
        # if taskpool.deadline <= cur_date then taskpool will be finished
        # by periodic task
        taskpool.status = TaskPool.Status.ACTIVE
        TaskPack.objects.filter(
            taskpool_id=taskpool.pk,
            status=TaskPack.Status.TIMEOUT
        ).update(
            status=TaskPack.Status.ACTIVE
        )
    else:
        if control == 'overlap':
            taskpool.status = TaskPool.Status.OVERLAPPED
        else:
            if control == 'disable':
                taskpool.status = TaskPool.Status.DISABLED
            else:
                # control == 'finish'
                taskpool.status = TaskPool.Status.FINISHED
            TaskPack.objects.filter(
                taskpool_id=taskpool.pk,
                status=TaskPack.Status.ACTIVE
            ).update(
                status=TaskPack.Status.TIMEOUT
            )
            Notification.objects.filter(
                kind=Notification.Kind.DEADLINE,
                taskpack__status=TaskPack.Status.TIMEOUT
            ).delete()
    taskpool.save()
    SteamLogger.info('%(act)s taskpool %(tp_id)d: %(tp_name)s with task_set_id %(ts_id)s',
                     type='ANG_TASKPOOL_EVENT', act=control,
                     tp_id=taskpool.id, tp_name=taskpool.title,
                     ts_id=taskpool.ang_taskset_id)
    return True


def recreate_estimations(taskpool_id):
    activation_time = timezone.now()
    # taskpool is disabled and locked =>
    # no one can change these ests
    Estimation.objects.filter(
        status__in=(Estimation.Status.ASSIGNED,
                    Estimation.Status.SKIPPED),
        task__taskpool_id=taskpool_id
    ).update(
        create_time=activation_time,
        start_time=ZERO_TIME,
        complete_time=ZERO_TIME
    )


def send_report(stats):
    status = ANGReport.Status.SENT
    try:
        acknowledgement = requests.post(
            ''.join((ANG_FRONT, '/external/api/statistics')),
            files={'filedata': stats}
        )
        ack_dict = acknowledgement.json()
        if ack_dict.get('response') == 'OK':
            status = ANGReport.Status.ACKNOWLEDGED
            SteamLogger.info('ANG statistics report sent',
                             type='ANG_REPORT_SUCCESS')
        else:
            SteamLogger.error('ANG statistics report failed: %(error)s',
                              type='ANG_REPORT_ERROR',
                              error=ack_dict.get('error'))
    except requests.ConnectionError as exc:
        SteamLogger.error('Could not receive response from ANG: %(exc)s',
                          type='ANG_REPORT_ERROR', exc=exc)
    except ValueError:
        SteamLogger.error(
            'Could not decode json from ANG response %(response)s',
            type='ANG_REPORT_ERROR', response=acknowledgement.text
        )
    return status
