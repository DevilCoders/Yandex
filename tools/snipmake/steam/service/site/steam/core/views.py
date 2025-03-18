# -*- coding: utf-8 -*-

from collections import Counter, defaultdict
from datetime import timedelta
import json
import os
import random
import re
import tempfile
import urllib

from celery.task.control import revoke
from django import template
from django.core.exceptions import ObjectDoesNotExist, PermissionDenied
from django.core.urlresolvers import resolve, reverse
from django.db import transaction, DatabaseError
from django.db.models import Q
from django.http import HttpResponse, HttpResponseBadRequest, Http404
from django.shortcuts import redirect, render, get_object_or_404
from django.utils import timezone

from django.utils.translation import (ugettext_noop as _,
                                      ugettext_lazy, string_concat)
from django.views.decorators.cache import never_cache

from django_yauth.util import get_passport_url

from ui.settings import USER_AUTH_REQUIRED, ASK_ANG_TASKPOOL_AVAILABILITY

from core.actions import (uploader, taskgenerator, estformer, packformer,
                          taskdealer, rawsql, estaggregator, timeprocessor,
                          ang, mailsender, mathstat, exporter, healthchecker,
                          sniptpl)
from core.actions.storage import Storage
from core.decorators import permission_required, retry
from core.forms import *
from core.hard.loghandlers import SteamLogger
from core.models import *
from core.settings import (TEMP_ROOT, URLS_LIST, ZERO_TIME)


GROUP_ALL = set([role[0] for role in User.Role.CHOICES])
GROUP_NO_AS = GROUP_ALL - {User.Role.ASSESSOR}
GROUP_NO_AS_AA = GROUP_NO_AS - {User.Role.AADMIN}
GROUP_DV = {User.Role.DEVELOPER}
GROUP_AS = {User.Role.ASSESSOR}


def alive(request):
    return HttpResponse('Peace')


@never_cache
def health(request):
    try:
        healthchecker.check_gluster()
        healthchecker.check_celeryd_and_rabbitmq()
        healthchecker.check_celerybeat()
        healthchecker.check_database()
        healthchecker.check_http500_rate()
        return HttpResponse('OK')
    except healthchecker.SteamHealthError as ex:
        return HttpResponse(str(ex), status=503)


@permission_required(GROUP_ALL)
def index(request):
    return render(request, 'core/index.html', {})


@permission_required(GROUP_ALL)
def help(request):
    return render(request, 'core/help.html', {})


@permission_required(GROUP_DV)
@retry()
@transaction.commit_on_success
def set_role(request, page):
    to_role = request.POST.get('role')
    to_status = request.POST.get('status')
    try:
        user = User.objects.get(
            pk=request.POST.get('user')
        )
    except ObjectDoesNotExist:
        to_role = None
        to_status = None

    if to_status and to_status in dict(User.Status.CHOICES):
        old_status = user.status
        user.status = to_status
        user.save()
        Notification.objects.filter(
            user=user
        ).delete()
        SteamLogger.info(
            '%(changer)s changed status of %(user)s from %(old)s to %(new)s',
            type='USER_MANAGEMENT_EVENT',
            changer=request.yauser.core_user.login, user=user.login,
            old=old_status, new=to_status
        )

    if to_role and to_role in dict(User.Role.CHOICES):
        if (
            user.role == User.Role.DEVELOPER and
            User.objects.filter(
                role=User.Role.DEVELOPER
            ).count() == 1
        ):
            redir = redirect('core:roles', page)
            redir['Location'] += '?error=nodevs'
            return redir
        if to_role == User.Role.ASSESSOR:
            # pack creation and estimation deletion
            packformer.create_packs(user)
        else:
            packformer.delete_incomplete_ests(user)
            packformer.delete_packs(user)
        old_role = user.role
        user.role = to_role
        user.save()
        SteamLogger.info(
            '%(changer)s changed role of %(user)s'
            ' from %(old_role)s to %(new_role)s',
            type='USER_MANAGEMENT_EVENT',
            changer=request.yauser.core_user.login, user=user.login,
            old_role=old_role, new_role=to_role
        )
        if (
            user == request.yauser.core_user and
            user.role != User.Role.DEVELOPER
        ):
            return redirect('/')
    return redirect('core:roles', page)


@permission_required(GROUP_NO_AS_AA)
def snippets(request, storage_id, page):
    try:
        snip_pool = SnippetPool.objects.get(pk=storage_id)
    except ObjectDoesNotExist:
        return redirect('core:snippetpools_default')
    cur_page = int(page)
    elements_on_page = 30
    search_request = request.GET.get('search_request', '')
    it = Storage.snippet_iterator(storage_id, cur_page * elements_on_page,
                                  (cur_page + 1) * elements_on_page,
                                  search_request)
    snip_tpls = sniptpl.get_snip_tpls(it, proto=True)
    pages = {'cur_page': cur_page,
             'obj_count': it.count(),
             'elements_on_page': elements_on_page,
             'query_string': request.META['QUERY_STRING']}
    return render(request, 'core/snippets.html',
                  {'snip_pool': snip_pool,
                   'snip_tpls': snip_tpls,
                   'pages': pages,
                   'search_request': search_request}
                  )


@permission_required(GROUP_NO_AS_AA)
def querybins(request, page):
    error = request.GET.get('error')
    cur_page = int(page)
    elements_on_page = 30
    query_bins = QueryBin.objects.all().order_by('-upload_time')[
        cur_page * elements_on_page:
        (cur_page + 1) * elements_on_page]
    pages = {'cur_page': cur_page,
             'obj_count': QueryBin.objects.count(),
             'elements_on_page': elements_on_page}
    popup_info_struct = {'open_button': ugettext_lazy('Add new'),
                         'handler_url': reverse('core:add_querybin',
                                                kwargs={'page': page}),
                         'header': ugettext_lazy('Add new query bin'),
                         'submit_button': ugettext_lazy('Save'),
                         'autoopen': False}
    form = UploadQueryBinForm()
    return render(request, 'core/querybins.html',
                  {'query_bins': query_bins, 'pages': pages,
                   'form': form, 'popup_info_struct': popup_info_struct,
                   'error': error})


@permission_required(GROUP_NO_AS)
def add_querybin(request, page):
    if request.method == 'POST':
        form = UploadQueryBinForm(request.POST, request.FILES)
        if form.is_valid():
            redir = redirect('core:querybins', page)
            if not uploader.process_querybin(request):
                redir['Location'] += '?error=unstorable'
            return redir
        cur_page = int(page)
        elements_on_page = 30
        query_bins = QueryBin.objects.all().order_by('-upload_time')[
            cur_page * elements_on_page:
            (cur_page + 1) * elements_on_page]
        pages = {'cur_page': cur_page,
                 'obj_count': QueryBin.objects.count(),
                 'elements_on_page': elements_on_page}
        popup_info_struct = {'open_button': ugettext_lazy('Add new'),
                             'handler_url': reverse('core:add_querybin',
                                                    kwargs={'page': page}),
                             'header': ugettext_lazy('Add new query bin'),
                             'submit_button': ugettext_lazy('Save'),
                             'autoopen': True}
        return render(request, 'core/querybins.html',
                      {'query_bins': query_bins, 'pages': pages,
                       'form': form, 'popup_info_struct': popup_info_struct})
    return redirect('core:querybins', page)


@permission_required(GROUP_NO_AS_AA)
def delete_querybin(request, qb_id, page):
    @retry(is_view=False)
    @transaction.commit_on_success
    def delete_action(qb_id):
        deleted_qb = get_object_or_404(QueryBin, pk=qb_id)
        same_link_qbs = QueryBin.objects.filter(
            storage_id=deleted_qb.storage_id
        ).count()
        deleted_qb.delete()
        return deleted_qb, same_link_qbs
    try:
        deleted_qb, same_link_qbs = delete_action(qb_id)
    except DatabaseError:
        return redirect('core:querybins', page)
    if same_link_qbs == 1:
        Storage.delete_file(deleted_qb.storage_id)
    return redirect('core:querybins', page)


@permission_required(GROUP_NO_AS_AA)
def snippetpools(request, page):
    cur_page = int(page)
    elements_on_page = 30
    snippet_pools = SnippetPool.objects.select_related(
        'user'
    ).order_by('-upload_time')[
        cur_page * elements_on_page:
        (cur_page + 1) * elements_on_page]
    pages = {'cur_page': cur_page,
             'obj_count': SnippetPool.objects.count(),
             'elements_on_page': elements_on_page}
    popup_info_struct = {'open_button': ugettext_lazy('Add new'),
                         'handler_url': reverse('core:add_snippetpool',
                                                kwargs={'page': page}),
                         'header': ugettext_lazy('Add new snippet pool'),
                         'submit_button': ugettext_lazy('Save'),
                         'autoopen': False}
    form = UploadSnippetPoolForm()
    upload_type = 'serp'
    return render(request, 'core/snippetpools.html',
                  {'snippet_pools': snippet_pools,
                   'pages': pages,
                   'form': form,
                   'upload_type': upload_type,
                   'popup_info_struct': popup_info_struct})


@permission_required(GROUP_NO_AS_AA)
@retry()
@transaction.commit_on_success
def add_snippetpool(request, page):
    if request.method == 'POST':
        form = UploadSnippetPoolForm(request.POST, request.FILES)
        upload_type = form.data.get('upload_type', 'serp')
        # form validation and BackgroundTask creation should be in one
        # transaction
        if form.is_valid():
            if upload_type == 'serp':
                uploader.download_serp(request)
            else:
                json_pool = form.cleaned_data['json_pool']
                uploader.process_snippetpool(request, json_pool)
            return redirect('core:monitor_default')
        cur_page = int(page)
        elements_on_page = 30
        snippet_pools = SnippetPool.objects.select_related(
            'user'
        ).order_by('title')[
            cur_page * elements_on_page:
            (cur_page + 1) * elements_on_page]
        pages = {'cur_page': cur_page,
                 'obj_count': SnippetPool.objects.count(),
                 'elements_on_page': elements_on_page}
        popup_info_struct = {'open_button': ugettext_lazy('Add new'),
                             'handler_url': reverse('core:add_snippetpool',
                                                    kwargs={'page': page}),
                             'header': ugettext_lazy('Add new snippet pool'),
                             'submit_button': ugettext_lazy('Save'),
                             'autoopen': True}
        return render(request, 'core/snippetpools.html',
                      {'snippet_pools': snippet_pools, 'pages': pages,
                       'form': form, 'upload_type': upload_type,
                       'popup_info_struct': popup_info_struct})
    return redirect('core:snippetpools', page)


@permission_required(GROUP_NO_AS_AA)
def delete_snippetpool(request, storage_id, page):
    @retry(is_view=False)
    @transaction.commit_on_success
    def delete_action(storage_id):
        delete_file = False
        if SnippetPool.can_delete_snippetpool(storage_id):
            delete_file = True
            SnippetPool.objects.filter(pk=storage_id).delete()
        return delete_file
    try:
        delete_file = delete_action(storage_id)
    except DatabaseError:
        delete_file = False
    if delete_file:
        Storage.delete_file(storage_id)
    return redirect('core:snippetpools', page)


@permission_required(GROUP_NO_AS_AA)
@retry()
@transaction.commit_on_success
def add_two_snippetpools(request):
    if request.method == 'POST':
        form = UploadTwoSnippetPoolsForm(request.POST, request.FILES)
        # form validation and BackgroundTask creation should be in one
        # transaction
        if form.is_valid():
            uploader.process_two_snippetpools(request)
            return redirect('core:monitor_default')
    else:
        form = UploadTwoSnippetPoolsForm()
    return render(request, 'core/add_two_snippetpools.html',
                  {'form': form})


@permission_required(GROUP_NO_AS)
def taskpools(request, tab='', page=0):
    got_tasks = request.COOKIES.get('got_tasks')
    error = request.GET.get('error')
    cur_page = int(page)
    elements_on_page = 30
    filter_status = request.GET.get('status', '')
    filter_country = request.GET.get('country', '')
    search_request = request.GET.get('search_request', '')
    non_ang_q = Q(ang_taskset_id='')
    start_idx = cur_page * elements_on_page
    end_idx = start_idx + elements_on_page
    taskpools_qs = TaskPool.objects.select_related(
        'first_pool',
        'second_pool',
        'user'
    ).prefetch_related(
        'tpcountry_set'
    ).filter(
        title__icontains=search_request,
    ).order_by(
        'priority', '-create_time'
    )
    if tab:
        taskpools_qs = taskpools_qs.exclude(non_ang_q)
    else:
        taskpools_qs = taskpools_qs.filter(non_ang_q)
    if filter_status in dict(TaskPool.Status.CHOICES):
        taskpools_qs = taskpools_qs.filter(status=filter_status)
    if filter_country in Country.CHOICES_DICT:
        taskpools_qs = taskpools_qs.filter(tpcountry__country=filter_country)
    active_tps_qs = taskpools_qs.filter(
        status__in=(TaskPool.Status.ACTIVE, TaskPool.Status.OVERLAPPED)
    )
    active_tps_count = active_tps_qs.count()
    active_tps, inactive_tps = [], []
    if active_tps_count > start_idx:
        active_tps = list(active_tps_qs[start_idx: end_idx])
        start_idx = active_tps_count
    if start_idx < end_idx:
        start_idx -= active_tps_count
        end_idx -= active_tps_count
        inactive_tps = list(taskpools_qs.exclude(
            status__in=(TaskPool.Status.ACTIVE,
                        TaskPool.Status.OVERLAPPED)
        )[start_idx: end_idx])
    pages = {'cur_page': cur_page,
             'obj_count': taskpools_qs.count(),
             'elements_on_page': elements_on_page,
             'query_string': request.META['QUERY_STRING']}
    response = render(request, 'core/taskpools.html',
                      {'taskpools': active_tps + inactive_tps,
                       'pages': pages,
                       'ang_tab': tab,
                       'got_tasks': int(got_tasks) if got_tasks else None,
                       'error': error,
                       'filter_status': filter_status,
                       'filter_country': filter_country,
                       'taskpool_statuses': TaskPool.Status.CHOICES,
                       'country_names': Country.CHOICES,
                       'search_request': search_request})
    response.delete_cookie('got_tasks')
    return response


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
def start_taskpool(request, taskpool_id):
    try:
        taskpool = TaskPool.objects.select_for_update().get(pk=taskpool_id)
    except ObjectDoesNotExist:
        raise Http404
    action = 'start'
    if request.method == 'POST':
        form = TaskPoolForm(request.POST, instance=taskpool,
                            action=action)
        if form.is_valid():
            form.save()
            ang.reset_taskpool(taskpool, action)
            return HttpResponse('')
        else:
            response_function = HttpResponseBadRequest
    else:
        form = TaskPoolForm(instance=taskpool, action=action)
        response_function = HttpResponse
    popup_info_struct = {
        'handler_url': reverse('core:start_taskpool',
                               kwargs={'taskpool_id': taskpool_id}),
        'header': ugettext_lazy('Start taskpool'),
        'submit_button': ugettext_lazy('Start'),
        'autoopen': True,
        'hide_button': True
    }
    tpl = template.loader.get_template('core/edit_taskpool_popup.html')
    return response_function(tpl.render(template.RequestContext(
        request, {'form': form,
                  'popup_info_struct': popup_info_struct})))


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
def stop_taskpool(request, action, taskpool_id, page):
    try:
        taskpool = TaskPool.objects.select_for_update().get(pk=taskpool_id)
    except ObjectDoesNotExist:
        return redirect('core:taskpools_default', tab='ang')
    ang.reset_taskpool(taskpool, action)
    return redirect('core:taskpools', tab='ang', page=page)


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
def add_taskpool(request):
    if request.method == 'POST':
        form = TaskPoolForm(request.POST)
        if form.is_valid():
            tasks_count = taskgenerator.generate(
                request, ignore_bold=form.cleaned_data['ignore_bold']
            )
            response = HttpResponse()
            response.set_cookie('got_tasks', tasks_count)
            return response
        else:
            response_function = HttpResponseBadRequest
    else:
        form = TaskPoolForm()
        response_function = HttpResponse
    popup_info_struct = {'handler_url': reverse('core:add_taskpool'),
                         'header': ugettext_lazy('Add new taskpool'),
                         'submit_button': ugettext_lazy('Save'),
                         'autoopen': True,
                         'hide_button': True}
    tpl = template.loader.get_template('core/add_taskpool_popup.html')
    kind_pool = form.data.get('kind_pool', TaskPool.TypePool.MULTI_CRITERIAL)
    return response_function(tpl.render(template.RequestContext(
        request, {'form': form,
                  'kind_pool' : kind_pool,
                  'popup_info_struct': popup_info_struct})))


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
def edit_taskpool(request, taskpool_id):
    taskpool = get_object_or_404(TaskPool, pk=taskpool_id)
    if request.method == 'POST':
        form = TaskPoolForm(request.POST, instance=taskpool)
        if form.is_valid():
            form.save()
            TPCountry.objects.filter(
                taskpool_id=taskpool_id
            ).delete()
            taskgenerator.define_countries(
                taskpool, request.POST.getlist('country')
            )
            Notification.objects.filter(
                kind=Notification.Kind.DEADLINE,
                taskpack__taskpool__deadline__gt=timezone.now() + timedelta(1)
            ).delete()
            return HttpResponse('')
        else:
            response_function = HttpResponseBadRequest
    else:
        form = TaskPoolForm(instance=taskpool)
        response_function = HttpResponse
    popup_info_struct = {
        'handler_url': reverse('core:edit_taskpool',
                               kwargs={'taskpool_id': taskpool_id}),
        'header': ugettext_lazy('Edit taskpool'),
        'submit_button': ugettext_lazy('Save'),
        'autoopen': True,
        'hide_button': True
    }
    tpl = template.loader.get_template('core/edit_taskpool_popup.html')
    return response_function(tpl.render(template.RequestContext(
        request, {'form': form,
                  'popup_info_struct': popup_info_struct})))


@permission_required(GROUP_NO_AS_AA)
@retry()
@transaction.commit_on_success
def delete_taskpool(request, taskpool_id, page):
    try:
        taskpool = TaskPool.objects.get(pk=taskpool_id)
    except ObjectDoesNotExist:
        return redirect('core:taskpools_default')
    ang_tab = 'ang' if taskpool.ang_taskset_id else ''
    if taskpool.can_delete():
        ang.reset_taskpool(taskpool, 'disable')

        tasks_to_delete = Task.objects.filter(taskpool_id=taskpool_id)
        snippet_to_delete = []
        for task in tasks_to_delete:
            snippet_to_delete.append(task.first_snippet)
            snippet_to_delete.append(task.second_snippet)

        #delete taskpool and сascade delete tasks, but not delete snippets
        taskpool.delete()
        #clean table core_snippets - delete snippets from deleted tasks
        for snippet in snippet_to_delete:
            if snippet.can_delete():
                snippet.delete()
    return redirect('core:taskpools', tab=ang_tab, page=page)


@permission_required(GROUP_NO_AS)
def export_taskpool(request, taskpool_id, page):
    ests_json = exporter.export_estimations(taskpool_id)
    if not ests_json:
        try:
            ang_taskset_id = TaskPool.objects.get(
                id=taskpool_id
            ).ang_taskset_id
        except ObjectDoesNotExist:
            ang_taskset_id = ''
        ang_tab = 'ang' if ang_taskset_id else ''
        redir = redirect('core:taskpools', tab=ang_tab, page=page)
        redir['Location'] += '?error=no_estimations'
        return redir
    response = HttpResponse(ests_json, content_type='application/json')
    response['Content-Disposition'] =\
        'attachment; filename=STEAM_estimations_%s.json' % taskpool_id
    return response


@permission_required(GROUP_NO_AS)
def view_taskpool(request, taskpool_id, order, page):
    cur_page = int(page)
    elements_on_page = 30
    try:
        taskpool = TaskPool.objects.select_related(
            'first_pool',
            'second_pool',
        ).get(pk=taskpool_id)
    except ObjectDoesNotExist:
        return redirect('core:taskpools_default')
    # show 'delete task' for tasks without estimations,
    # but also can be processed for tasks only with timeout-estimations
    tasks = Task.objects.select_related(
        'first_snippet',
        'second_snippet',
    ).filter(
        taskpool=taskpool_id
    ).annotate(
        est_count=Count('estimation')
    )
    if order == 'random':
        tasks = tasks.extra(
            select={'hash':
                    'MD5(CONCAT(first_snippet_id, second_snippet_id))'},
            order_by=('hash',)
        )
    tasks = tasks[
        cur_page * elements_on_page:
        (cur_page + 1) * elements_on_page
    ].annotate(
        est_count=Count('estimation')
    )
    for task in tasks:
        # tasks are not saved!
        # we need to cancel shuffling for pool-aligned rendering
        task.shuffle = False
    pages = {'cur_page': cur_page,
             'obj_count': Task.objects.filter(taskpool=taskpool_id).count(),
             'elements_on_page': elements_on_page}
    return render(request, 'core/task_list.html',
                  {'taskpool': taskpool,
                   'tasks_ext': [(task, sniptpl.get_snip_tpls_ext(task))
                                 for task in tasks],
                   'pages': pages,
                   'order': order,
                   'error': request.GET.get('error')})


@permission_required(GROUP_NO_AS)
def monitor_taskpool(request, taskpool_id):
    try:
        taskpool = TaskPool.objects.get(pk=taskpool_id)
    except ObjectDoesNotExist:
        return redirect('core:taskpools_default')
    custom_ests = Estimation.objects.filter(
        taskpack__taskpool_id=taskpool_id
    ).values(
        'user__login',
        'status',
        'taskpack__status',
        'taskpack__last_update',
    ).annotate(
        status_count=Count('status')
    )
    taskpacks_info = {}
    taskpack_statuses = dict(TaskPack.Status.CHOICES)
    for est in custom_ests:
        login = est['user__login']
        if login not in taskpacks_info:
            taskpacks_info[login] = {
                'complete': 0,
                'rejected': 0,
                'skipped': 0,
                'total': 0,
                'linked': 0,
                'pack_status': taskpack_statuses[est['taskpack__status']],
                'last_update': est['taskpack__last_update'],
            }
        if est['status'] == Estimation.Status.COMPLETE:
            taskpacks_info[login]['complete'] = est['status_count']
            taskpacks_info[login]['total'] += est['status_count']
        elif est['status'] == Estimation.Status.REJECTED:
            taskpacks_info[login]['rejected'] = est['status_count']
            taskpacks_info[login]['total'] += est['status_count']
        elif est['status'] == Estimation.Status.SKIPPED:
            taskpacks_info[login]['skipped'] = est['status_count']
    est_vals = Estimation.objects.filter(
        taskpack__taskpool_id=taskpool_id,
        status__in=(Estimation.Status.COMPLETE, Estimation.Status.REJECTED)
    ).values(
        'user__login',
        'json_value'
    )
    for est_val in est_vals:
        try:
            if json.loads(est_val['json_value'],
                          encoding='utf-8').get('linked'):
                taskpacks_info[est_val['user__login']]['linked'] += 1
        except (TypeError, ValueError):
            continue
    return render(request, 'core/monitor_taskpool.html',
                  {'taskpool': taskpool,
                   'taskpacks_info': taskpacks_info})


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
def delete_task(request, task_id, order, page):
    try:
        task = Task.objects.select_related(
            'taskpool',
            'first_snippet',
            'second_snippet',
        ).get(
            pk=task_id
        )
    except ObjectDoesNotExist:
        return redirect('core:taskpools_default')
    redir = redirect('core:view_taskpool', taskpool_id=task.taskpool_id,
                     order=order, page=page)
    if Estimation.objects.filter(
        task_id=task_id
    ).exclude(
        status=Estimation.Status.TIMEOUT
    ).count():
        redir['Location'] += '?error=hasests'
    else:
        taskpool = task.taskpool
        first_snippet = task.first_snippet
        second_snippet = task.second_snippet
        task.delete()
        taskpool.count -= 1
        if taskpool.count > 0:
            taskpool.pack_size = min(taskpool.pack_size, taskpool.count)
            taskpool.save()
        else:
            ang.reset_taskpool(taskpool, 'disable')
            taskpool.delete()
            redir = redirect('core:taskpools_default')
        for snippet in (first_snippet, second_snippet):
            if snippet.can_delete():
                snippet.delete()
    return redir


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
# nothing is altered => locks not needed
def tasks(request, taskpool_id, status, order, page):
    search_request = request.GET.get('search_request', '')
    cur_page = int(page)
    elements_on_page = 30
    try:
        taskpool = TaskPool.objects.get(pk=taskpool_id)
    except ObjectDoesNotExist:
        return redirect('core:taskpools_default')
    tasks = rawsql.get_tasks_range(taskpool_id,
                                   cur_page * elements_on_page,
                                   elements_on_page,
                                   search_request,
                                   status, order)
    complete_ests = Estimation.objects.filter(
        pk__in=[task.complete_est for task in tasks],
        task__status=Task.Status.COMPLETE,
    ).select_related(
        'correction',
        'correction__aadmin_est__user'
    )
    aadmins = {}
    for est in complete_ests:
        if est.correction:
            aadmins[est.task_id] = est.correction.aadmin_est.user.login
    tasks_with_aadmins = [(task, aadmins.get(task.id)) for task in tasks]
    tasks_not_to_assign = Estimation.objects.filter(
        task__taskpool_id=taskpool_id,
        user=request.yauser.core_user,
    ).exclude(
        status__in=(Estimation.Status.TIMEOUT,
                    Estimation.Status.REJECTED)
    ).values_list('task_id', flat=True)
    filter_kwargs = {'taskpool_id': taskpool_id,
                     'request__icontains': search_request}
    if status == 'inspection':
        filter_kwargs['status__in'] = (Task.Status.INSPECTION,
                                       Task.Status.COMPLETE)
    pages = {'cur_page': cur_page,
             'obj_count': Task.objects.filter(**filter_kwargs).count(),
             'elements_on_page': elements_on_page,
             'query_string': request.META['QUERY_STRING']}
    return render(request, 'core/tasks.html',
                  {'taskpool': taskpool,
                   'tasks_with_aadmins': tasks_with_aadmins,
                   'tasks_not_to_assign': tasks_not_to_assign,
                   'enable_taking': not (
                       request.yauser.core_user.role == User.Role.AADMIN and
                       taskpool.status == TaskPool.Status.DISABLED
                   ),
                   'pages': pages, 'search_request': search_request,
                   'status': status, 'order': order})


@permission_required(GROUP_ALL)
@retry()
@transaction.commit_on_success
def estimation(request, est_id, exp_criterion=None):
    user = request.yauser.core_user
    redir = estformer.get_redirect(user, est_id, exp_criterion)
    if 'error' in redir['Location']:
        return redir

    # est exists (checked in estformer.get_redirect)
    #
    # every change on ests connected with this task is prohibited
    # because of serializable isolation level
    est = Estimation.objects.select_related(
        'task',
        'task__taskpool',
        'task__taskpool__first_pool',
        'task__taskpool__second_pool',
        'task__first_snippet',
        'task__second_snippet',
        'user',
        'taskpack',
        'taskpack__taskpool'
    ).get(pk=est_id)

    (snippet_rendering_kwargs, show_request,
     criterion, prev_criterion, next_criterion) = \
        estformer.criterion_based_values(est, user, exp_criterion)
    snippets = est.get_snippets(**snippet_rendering_kwargs)
    cur_tpls = est.cur_tpls()
    snip_pool_1 = estformer.get_pool_if_exists(snippets[0])
    snip_pool_2 = estformer.get_pool_if_exists(snippets[1])
    est_can_be_changed = (
        # only est owner can change its value
        user == est.user and
        # if he didn't reject this estimation and estimation is not TIMEOUT
        est.status not in (Estimation.Status.REJECTED,
                           Estimation.Status.TIMEOUT) and
        # estimation should be from open taskpool
        (est.task.taskpool.status in (TaskPool.Status.ACTIVE,
                                      TaskPool.Status.OVERLAPPED) or
         # or user should have permissions to take tasks from DISABLED TPs
         user.role in (User.Role.ANALYST, User.Role.DEVELOPER) or
         # user is not an assessor and pool is FINISHED
         (not est.taskpack_id
          and est.task.taskpool.status == TaskPool.Status.FINISHED) or
         # user is an assessor who has a re-enabled TaskPack
         (est.taskpack_id and est.taskpack.status == TaskPack.Status.ACTIVE))
    )
    if request.method == 'POST':
        if user == est.user:
            if not est_can_be_changed:
                return redirect('core:estimation', est_id)
            form = EstimationForm(request.POST, criterion=criterion, kind_pool=est.task.taskpool.kind_pool)
            if form.is_valid():
                estformer.finish_estimation(request, est, criterion=criterion)
                return estformer.redirect_after_estimation(request, est)
        else:
            if est.status != Estimation.Status.SKIPPED:
                return redirect('core:usertasks_default', 'questions')
            form = AnswerEstimationForm(request.POST)
            if form.is_valid():
                estformer.answer_question(request, est)
                return redirect('core:usertasks_default', 'questions')
    else:
        if user == est.user:
            initial = {'time_elapsed': est.get_elapsed_time(),
                       'comment': est.comment}
            if est.status == Estimation.Status.COMPLETE or exp_criterion:
                initial.update(est.pack_value(est.json_value))
            form = EstimationForm(initial=initial, criterion=criterion, kind_pool=est.task.taskpool.kind_pool)
        else:
            form = AnswerEstimationForm()
    return render(request, 'core/estimation.html',
                  {'est': est,
                   'snip_tpls_ext': zip(sniptpl.get_snip_tpls(snippets),
                                        cur_tpls),
                   'snip_pool_1': snip_pool_1,
                   'snip_pool_2': snip_pool_2, 'est_form': form,
                   'show_request': show_request,
                   'est_can_be_changed': est_can_be_changed,
                   'criterion': criterion,
                   'prev_criterion': prev_criterion,
                   'next_criterion': next_criterion})


@permission_required(GROUP_ALL)
def usertasks(request, tab, page):
    tabs = (_('current'), _('finished'),
            _('checked'), _('questions'))
    user = request.yauser.core_user
    if user.role == user.Role.ASSESSOR:
        if tab == tabs[3]:
            raise PermissionDenied
        else:
            tabs = tabs[:-1]

    error = request.GET.get('error', '')
    unfolded = request.GET.get('unfold', '')
    cur_page = int(page)
    elements_on_page = 30

    filter_Q = Q(user=user)
    objects = Estimation.objects.select_related(
        'task',
        'task__taskpool',
        'user',
        'taskpack',
    )
    if tab == tabs[0]:
        if user.role == User.Role.ASSESSOR:
            objects = TaskPack.objects.select_related(
                'taskpool'
            )
            filter_Q &= Q(status=TaskPack.Status.ACTIVE)
            sort_order = ('taskpool__deadline',)
        else:
            filter_Q &= Q(status__in=(Estimation.Status.ASSIGNED,
                                      Estimation.Status.SKIPPED))
            sort_order = ('status', '-create_time')
    elif tab == tabs[1]:
        if user.role == User.Role.ASSESSOR:
            objects = TaskPack.objects.select_related(
                'taskpool'
            )
            filter_Q &= Q(status__in=(TaskPack.Status.FINISHED,
                                      TaskPack.Status.TIMEOUT))
            sort_order = ('-taskpool__deadline',)
        else:
            filter_Q &= Q(status__in=(Estimation.Status.REJECTED,
                                      Estimation.Status.TIMEOUT,
                                      Estimation.Status.COMPLETE))
            sort_order = ('-complete_time',)
    elif tab == tabs[2]:
        filter_Q &= (qconstructor.complete_est_q() &
                     Q(task__status=Task.Status.COMPLETE))
        sort_order = ('-correction__time',)
        # tested looking at queries:
        # A.select_related(l1).select_related(l2)
        # fetches only related objects from l2.
        # So, specifying every related object again
        objects = objects.select_related(
            'task',
            'task__taskpool',
            'user',
            'correction',
            'correction__aadmin_est',
            'correction__aadmin_est__task',
        )
    elif tab == tabs[3]:
        filter_Q = Q()
        filter_Q &= Q(status=Estimation.Status.SKIPPED)
        sort_order = ('-create_time',)

    objects_query = objects.filter(
        filter_Q
    ).order_by(
        *sort_order
    )
    rendered_objects = objects_query[cur_page * elements_on_page:
                                     (cur_page + 1) * elements_on_page]
    obj_count = objects_query.count()
    pages = {'cur_page': cur_page,
             'obj_count': obj_count,
             'elements_on_page': elements_on_page}
    tab_available = _('available')
    return render(request, 'core/usertasks.html',
                  {'rendered_objects': rendered_objects,
                   'pages': pages, 'tabs': tabs,
                   'tab': tab, 'tab_available': tab_available,
                   'hide_disabled_links': user.role not in (
                       User.Role.ANALYST, User.Role.DEVELOPER
                   ),
                   'unfolded': unfolded,
                   'error': error})


@permission_required(GROUP_AS)
def usertask_packs(request, page):
    user = request.yauser.core_user
    cur_page = int(page)
    error = request.GET.get('error', '')
    elements_on_page = 30
    taskpool_qs = TaskPool.objects.filter(
        status=TaskPool.Status.ACTIVE,
        tpcountry__country=user.language,
        deadline__gt=timezone.localtime(timezone.now()).date(),
    ).exclude(
        taskpack__user_id=user.pk
    ).order_by(
        'deadline'
    )
    if ASK_ANG_TASKPOOL_AVAILABILITY:
        taskpool_qs = taskpool_qs.filter(
            ang_taskset_id__in=ang.available_taskset_ids(user)
        )
    taskpools = taskpool_qs[cur_page * elements_on_page:
                            (cur_page + 1) * elements_on_page]
    taskpool_count = taskpool_qs.count()
    pages = {'cur_page': cur_page,
             'obj_count': taskpool_count,
             'elements_on_page': elements_on_page}
    tabs = (_('current'), _('finished'), _('checked'))
    tab_available = _('available')
    return render(request, 'core/usertask_packs.html',
                  {'taskpools': taskpools, 'pages': pages,
                   'tabs': tabs, 'tab_available': tab_available,
                   'error': error})


@permission_required(GROUP_AS)
@retry()
@transaction.commit_on_success
def take_user_tasks(request, taskpool_id, page):
    redir = redirect('core:usertask_packs', page)
    user = request.yauser.core_user
    try:
        taskpool = TaskPool.objects.exclude(
            taskpack__user_id=user.pk
        ).get(
            pk=taskpool_id,
            status=TaskPool.Status.ACTIVE,
            tpcountry__country=user.language,
            deadline__gt=timezone.localtime(timezone.now()).date(),
        )
    except ObjectDoesNotExist:
        redir['Location'] += '?error=inexistent'
        return redir
    if (
        ASK_ANG_TASKPOOL_AVAILABILITY and
        int(taskpool.ang_taskset_id) not in ang.available_taskset_ids(user)
    ):
        redir['Location'] += '?error=inexistent'
        return redir
    pack_id = taskdealer.create_taskpack(user, taskpool)
    if pack_id is None:
        redir['Location'] += '?error=collision'
        ang.reset_taskpool(taskpool, 'overlap')
        return redir
    # queryset from usertasks/current/ page for assessor
    taskpack_ids = list(TaskPack.objects.filter(
        user=user,
        taskpool__status=TaskPool.Status.ACTIVE
    ).order_by(
        'taskpool__deadline'
    ).values_list('id', flat=True))
    # TODO: make a global constant
    elements_on_page = 30
    page_in_current = taskpack_ids.index(pack_id) / elements_on_page
    redir = redirect('core:usertasks', 'current', page_in_current)
    redir['Location'] += '?unfold=%s' % taskpool_id
    return redir


@permission_required(GROUP_NO_AS)
def available_usertasks(request, page):
    search_request = request.GET.get('search_request', '')

    error = request.GET.get('error', '')
    cur_page = int(page)
    elements_on_page = 30
    available_tasks_query = Task.objects.filter(
        taskpool__status=TaskPool.Status.FINISHED,
        status=Task.Status.INSPECTION,
        taskpool__tpcountry__country=request.yauser.core_user.language,
        request__icontains=search_request,
    ).exclude(
        estimation__user=request.yauser.core_user,
        estimation__status__in=(Estimation.Status.REJECTED,
                                Estimation.Status.ASSIGNED,
                                Estimation.Status.COMPLETE,
                                Estimation.Status.SKIPPED,)
    ).order_by('-taskpool__create_time')

    available_tasks = available_tasks_query[
        cur_page * elements_on_page:
        (cur_page + 1) * elements_on_page
    ]
    available_tasks_count = available_tasks_query.count()
    pages = {'cur_page': cur_page,
             'obj_count': available_tasks_count,
             'elements_on_page': elements_on_page,
             'query_string': request.META['QUERY_STRING']}
    tabs = (_('current'), _('finished'),
            _('checked'), _('questions'))
    tab_available = _('available')
    return render(request, 'core/availabletasks.html',
                  {'available_tasks': available_tasks,
                   'pages': pages, 'tabs': tabs,
                   'tab_available': tab_available, 'error': error,
                   'search_request': search_request})


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
def take_aadmin_task(request, task_id):
    (tasks, aa_ests_ext) = taskdealer.take_aadmin_task_batch(
        request.yauser.core_user, (task_id,))
    if not tasks:
        redir = redirect('core:available_usertasks_default')
        redir['Location'] += '?error=collision'
        return redir
    est = aa_ests_ext[0]
    if est.status == Estimation.Status.COMPLETE:
        return redirect('core:estimation_check', est.id)
    else:
        return redirect('core:estimation', est.id)


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
def take_aadmin_task_batch(
        request, action, taskpool_id=0, status='all', order='alphabet', page=0
):
    task_ids = request.POST.getlist('task_ids')
    if action == 'take':
        redir = redirect('core:available_usertasks', page)
        taskdealer_function = taskdealer.take_aadmin_task_batch
    else:
        redir = redirect('core:tasks', taskpool_id, status, order, page)
        taskdealer_function = lambda user, ids: taskdealer.assign_task_batch(
            user, taskpool_id, ids
        )
    if task_ids:
        (tasks, aa_ests_ext) = taskdealer_function(request.yauser.core_user,
                                                   task_ids)
        if not tasks:
            redir['Location'] += '?error=collision'
    return redir


@permission_required(GROUP_NO_AS_AA)
def monitor(request, page):
    cur_page = int(page)
    elements_on_page = 30
    bgtasks = BackgroundTask.objects.select_related(
        'user'
    ).order_by('-start_time')[
        cur_page * elements_on_page:
        (cur_page + 1) * elements_on_page
    ]
    pages = {'cur_page': cur_page,
             'obj_count': BackgroundTask.objects.count(),
             'elements_on_page': elements_on_page}
    return render(request, 'core/monitor.html', {'bgtasks': bgtasks,
                                                 'pages': pages})


@permission_required(GROUP_NO_AS_AA)
def monitor_statuses(request, page):
    cur_page = int(page)
    elements_on_page = 30
    bgtasks = BackgroundTask.objects.select_related(
        'user'
    ).order_by('-start_time')[
        cur_page * elements_on_page:
        (cur_page + 1) * elements_on_page
    ]
    return render(request, 'core/monitor_body.html', {'bgtasks': bgtasks})


def request_permission(request):
    user = request.yauser.core_user
    if user and user.status != User.Status.WAIT_APPROVE:
        return redirect('core:index')
    form = AddUserForm()
    popup_info_struct = {'open_button': ugettext_lazy('Request permissions'),
                         'handler_url': reverse('core:add_user'),
                         'header': ugettext_lazy('Request permissions'),
                         'submit_button': ugettext_lazy('Submit'),
                         'autoopen': False}
    return render(request, 'core/request_permission.html',
                  {'is_user': user is not None,
                   'form': form,
                   'popup_info_struct': popup_info_struct})


def add_user(request):
    user = request.yauser.core_user
    if user and user.status != User.Status.WAIT_APPROVE:
        return redirect('core:index')
    if request.method == 'POST':
        form = AddUserForm(request.POST)
        if form.is_valid():
            user = User(login=request.yauser.login,
                        language=request.POST['language'],
                        status=User.Status.WAIT_APPROVE,
                        yandex_uid=request.yauser.uid)
            user.save()
            SteamLogger.info('%(user)s requested permissions',
                             type='USER_MANAGEMENT_EVENT', user=user.login)
            mailsender.notify_about_user(request)
            mailsender.email_to_new_user(request)
            return redirect('core:request_permission')
        popup_info_struct = {
            'open_button': ugettext_lazy('Request permissions'),
            'handler_url': reverse('core:add_user'),
            'header': ugettext_lazy('Request permissions'),
            'submit_button': ugettext_lazy('Submit'),
            'autoopen': True
        }
        return render(request, 'core/request_permission.html',
                      {'is_user': request.yauser.core_user is not None,
                       'form': form,
                       'popup_info_struct': popup_info_struct})
    return redirect('core:request_permission')


@permission_required(GROUP_NO_AS)
def ajax_fileupload(request):
    handle, tmp_name = tempfile.mkstemp(dir=TEMP_ROOT)
    try:
        os.write(handle, request.FILES['file'].read())
    except KeyError:
        return HttpResponseBadRequest('No file specified!')
    os.close(handle)
    return HttpResponse(os.path.basename(tmp_name))


@permission_required(GROUP_NO_AS)
def ajax_filedelete(request):
    try:
        filename = request.POST['file']
        if not re.search(r'^(?:\w|_)+$', filename):
            raise PermissionDenied
    except KeyError:
        return HttpResponseBadRequest('No file specified!')
    try:
        os.remove(os.path.join(TEMP_ROOT, filename))
    except OSError:
        pass
    return HttpResponse(filename)


@permission_required(GROUP_NO_AS_AA)
def stop_bg_task(request, celery_id):
    filter_kwargs = {'pk': celery_id}
    if request.yauser.core_user.role != User.Role.DEVELOPER:
        filter_kwargs['user'] = request.yauser.core_user
    if BackgroundTask.objects.exclude(
        status__in=('SUCCESS', 'ERROR', 'STOPPED')
    ).filter(**filter_kwargs).update(status=_('STOPPED')):
        revoke(celery_id, terminate=True)
        return HttpResponse(celery_id)
    else:
        raise PermissionDenied


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
def estimation_check(request, est_id):
    try:
        # every change on ests connected with this task is prohibited
        # because of serializable isolation level
        user_est = Estimation.objects.select_related(
            'task',
            'task__taskpool',
            'task__taskpool__first_pool',
            'task__taskpool__second_pool',
            'task__first_snippet',
            'task__second_snippet',
            'correction',
            'correction__aadmin_est',
            'correction__aadmin_est__task',
        ).prefetch_related(
            'aadmin_corrections'
        ).exclude(
            task__status=Task.Status.REGULAR
        ).get(
            user=request.yauser.core_user,
            status=Estimation.Status.COMPLETE,
            task__taskpool__status=TaskPool.Status.FINISHED,
            pk=est_id
        )
    except ObjectDoesNotExist:
        redir = redirect('core:usertasks_default', tab='finished')
        redir['Location'] += '?error=uncheckable'
        return redir
    error = request.GET.get('error', '')
    if user_est.task.status == Task.Status.COMPLETE:
        aadmin_est = user_est.correction.aadmin_est
    else:
        aadmin_est = user_est
    actual_only = aadmin_est != user_est
    snippets = user_est.task.snippets()
    cur_tpls = user_est.task.cur_tpls()
    other_ests = Estimation.objects.select_related(
        'user',
        'task',
        'correction',
        'correction__aadmin_est',
        'correction__aadmin_est__task',
    ).exclude(pk=est_id).filter(
        qconstructor.complete_est_q(),
        task=user_est.task
    )
    draft = (
        user_est.task.status != Task.Status.COMPLETE or (
            not actual_only and
            user_est.correction_by_aadmin_est(user_est).status ==
            Correction.Status.SAVED
        )
    )
    (assessor_ests, counted_ests) = (
        estaggregator.aggregate_by_assessor(other_ests, actual_only, user_est),
        estaggregator.aggregate_by_count(other_ests, shuffle=True)
    )
    if request.method == 'POST':
        est_form = EstimationForm(request.POST, kind_pool=user_est.task.taskpool.kind_pool)
        if est_form.is_valid() and request.POST['skipped'] == 'False':
            if (
                # pack_value used for ignoring non-criterion fields in json
                aadmin_est.pack_value(aadmin_est.json_value) ==
                aadmin_est.pack_value(request.POST, ignore_shuffle=True)
            ):
                # reference estimation remains unchanged
                aadmin_est.comment = request.POST.get('comment', '')
                aadmin_est.save()
            else:
                estformer.finish_estimation(request, user_est)
            return redirect('core:estimation_check', est_id=est_id)
    else:
        initial_dict = dict(zip(aadmin_est.get_criterions_names(),
                                aadmin_est.digits(shuffle=True)))
        initial_dict['comment'] = aadmin_est.comment
        initial_dict['skipped'] = 'False'
        est_form = EstimationForm(initial=initial_dict, kind_pool=user_est.task.taskpool.kind_pool)
    return render(request, 'core/estimation_check.html', {
        'est': aadmin_est,
        'snip_tpls_ext': zip(sniptpl.get_snip_tpls(snippets), cur_tpls),
        'error': error,
        'snip_pool_1': estformer.get_pool_if_exists(snippets[0]),
        'snip_pool_2': estformer.get_pool_if_exists(snippets[1]),
        'assessor_ests': assessor_ests, 'counted_ests': counted_ests,
        'est_form': est_form, 'draft': draft, 'est_id': est_id,
        'showMediaContent': aadmin_est.task.taskpool.kind_pool == TaskPool.TypePool.RCA,
    })


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
def process_errors(request, est_id):
    try:
        # every change on ests connected with this task is prohibited
        # because of serializable isolation level
        user_est = Estimation.objects.select_related(
            'task',
            'correction',
            'correction__aadmin_est',
        ).prefetch_related(
            'aadmin_corrections'
        ).exclude(
            task__status=Task.Status.REGULAR
        ).get(
            user=request.yauser.core_user,
            status=Estimation.Status.COMPLETE,
            task__taskpool__status=TaskPool.Status.FINISHED,
            pk=est_id
        )
    except ObjectDoesNotExist:
        return redirect('core:estimation_check', est_id)
    # if task is in Complete status
    # aadmin_est is Estimation of AA who made latest correction
    if user_est.task.status == Task.Status.COMPLETE:
        aadmin_est = user_est.correction.aadmin_est
    else:
        aadmin_est = user_est
    try:
        request_est_id = int(request.POST.get('aadmin_est_number', ''))
    except ValueError:
        request_est_id = None
    if request_est_id != aadmin_est.id:
        redir = redirect('core:estimation_check', est_id)
        redir['Location'] += '?error=collision'
        return redir
    aadmin_digits = aadmin_est.digits()
    assessor_ests = Estimation.objects.filter(
        qconstructor.complete_est_q(),
        task=user_est.task
    ).exclude(pk=est_id)
    redir = None
    new_est_left = False
    est_ids = [est_id]
    for assessor_est in assessor_ests:
        if not request.POST.get('error_visible_%d' % assessor_est.id):
            new_est_left = True
            redir = redirect('core:estimation_check', est_id)
            redir['Location'] += '?error=new_est_left'
            break
        est_ids.append(assessor_est.id)
    action = request.POST.get('action')
    correction_batch_time = timezone.now()
    if action == 'Next' and not new_est_left:
        status = Correction.Status.ACTUAL
        user_est.task.status = Task.Status.COMPLETE
        user_est.task.save()
        if request.POST.get('compose') == 'on' and assessor_ests:
            redir = redirect('core:send_email', est_id)
    else:
        status = Correction.Status.SAVED
    # kill old SAVED corrections: they are not needed anymore
    Correction.objects.filter(
        status=Correction.Status.SAVED,
        aadmin_est_id=aadmin_est.id,
        assessor_est_id__in=est_ids,
    ).delete()
    # create correction for the checking AA without errors
    # to ensure everybody who solved this task has latest correction
    corrections = [Correction(aadmin_est=aadmin_est, assessor_est=user_est,
                              time=correction_batch_time, errors=0,
                              comment='', status=status)]
    for assessor_est in assessor_ests:
        if request.POST.get('error_visible_%d' % assessor_est.id):
            correction = Correction(aadmin_est=aadmin_est,
                                    assessor_est=assessor_est,
                                    time=correction_batch_time, errors=0,
                                    comment='', status=status)
            assessor_digits = assessor_est.digits()
            # We provide the following invariant:
            # if inspection task is checked by AA
            # 1. Every Complete estimation has a correction
            # 2. The value in correction is accurate and when
            #    we collect stats we don't need to check if digits
            #    really differ in AA and AS ests.
            for i in range(len(assessor_est.get_criterions_names())):
                if (
                    request.POST.get('error_checkbox_%d_%d' %
                                    (assessor_est.id, i)) == 'on' and
                    assessor_digits[i] != aadmin_digits[i]
                ):
                    correction.errors |= 1 << i
            corrections.append(correction)
    Correction.objects.bulk_create(corrections)
    if status == Correction.Status.ACTUAL:
        rawsql.make_correction_relations(aadmin_est, est_ids,
                                         correction_batch_time)
    if not redir:
        redir = estformer.redirect_after_check(request.yauser.core_user)
    return redir


@permission_required(GROUP_ALL)
@retry()
@transaction.commit_on_success
def corrections(request, tab, task_id):
    tabs = (_('all'), _('significant'))
    try:
        user_est = Estimation.objects.select_related(
            'user',
            'task',
            'task__first_snippet',
            'task__second_snippet',
            'correction',
            'correction__aadmin_est',
            'correction__aadmin_est__task',
        ).get(
            qconstructor.complete_est_q(),
            user=request.yauser.core_user,
            task_id=task_id,
            task__status=Task.Status.COMPLETE
        )
    except ObjectDoesNotExist:
        redir = redirect('core:usertasks_default', tab='checked')
        redir['Location'] += '?error=notchecked'
        return redir
    correction = user_est.correction
    if not correction:
        redir = redirect('core:usertasks_default', tab='checked')
        redir['Location'] += '?error=notchecked'
        return redir
    aadmin_est = correction.aadmin_est
    if request.yauser.core_user.role == User.Role.ASSESSOR:
        ests = (user_est,)
    else:
        ests = Estimation.objects.select_related(
            'user',
            'task',
            'correction',
            'correction__aadmin_est',
            'correction__aadmin_est__task',
        ).filter(
            qconstructor.complete_est_q(),
            task_id=task_id
        ).exclude(pk=user_est.id)
    render_context = {'task': user_est.task, 'aadmin_est': aadmin_est,
                      'tabs': tabs, 'tab': tab}
    render_context['snip_tpls_ext'] = sniptpl.get_snip_tpls_ext(user_est.task)
    if request.yauser.core_user.role != User.Role.ASSESSOR:
        render_context['snip_pool_1'] = estformer.get_pool_if_exists(
            user_est.task.first_snippet
        )
        render_context['snip_pool_2'] = estformer.get_pool_if_exists(
            user_est.task.second_snippet
        )
    assessor_ests = estaggregator.aggregate_by_assessor(ests, actual_only=True)
    if tab == 'significant':
        assessor_ests = [
            as_est for as_est in assessor_ests
            if as_est[0].correction.errors
        ]
    render_context['assessor_ests'] = assessor_ests
    return render(request, 'core/corrections.html', render_context)


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
def send_email(request, est_id):
    try:
        user_est = Estimation.objects.select_related(
            'user',
            'task',
            'task__first_snippet',
            'task__second_snippet',
            'correction',
            'correction__aadmin_est',
        ).get(
            pk=est_id,
            user=request.yauser.core_user,
            status=Estimation.Status.COMPLETE,
            task__status=Task.Status.COMPLETE
        )
        aadmin_est = user_est.correction.aadmin_est
        ests = Estimation.objects.select_related(
            'user',
            'task',
        ).filter(
            qconstructor.complete_est_q(),
            task_id=aadmin_est.task_id
        ).exclude(pk=est_id)
        if not ests:
            raise ObjectDoesNotExist
    except ObjectDoesNotExist:
        redir = redirect('core:usertasks_default', tab='finished')
        redir['Location'] += '?error=notchecked'
        return redir
    snip_tpls_ext = sniptpl.get_snip_tpls_ext(user_est.task)
    url = user_est.task.first_snippet.data['url']
    form = EmailForm(initial={'subject': string_concat(
        ugettext_lazy('Estimation'), ': ', user_est.task.request, ' url: ', url
    )})
    recipients = [est.user.login for est in ests]
    error = ''
    if request.method == 'POST':
        form = EmailForm(request.POST)
        try:
            recipients = form.get_recipients()
        except ValidationError:
            pass
        if form.is_valid():
            if (
                mailsender.send_all_emails(request, user_est,
                                           recipients, snip_tpls_ext)
            ):
                return estformer.redirect_after_check(
                    request.yauser.core_user
                )
            else:
                error = _('An error occured. Please try again later.')

    return render(request, 'core/send_email.html', {
        'ests': ests, 'aadmin_est': user_est,
        'form': form, 'recipients': recipients,
        'url': url, 'error': error,
        'all_recip': len(ests) == len(recipients),
        'show_pools': False, 'snip_tpls_ext': snip_tpls_ext,
        'counted_ests': estaggregator.aggregate_by_count(ests, shuffle=True),
        'showMediaContent':  user_est.task.taskpool.kind_pool == TaskPool.TypePool.RCA,
    })


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
# nothing is altered => locks not needed
def statistics(request, taskpool_id, criterion, value_name):
    try:
        taskpool = TaskPool.objects.select_related(
            'first_pool',
            'second_pool'
        ).get(
            pk=taskpool_id
        )
    except ObjectDoesNotExist:
        redir = redirect('core:taskpools_default')
        redir['Location'] += '?error=inexistent'
        return redir
    ests = list(Estimation.objects.select_related(
        'task',
        'task__first_snippet',
        'task__second_snippet',
        'correction',
        'correction__aadmin_est'
    ).filter(
        qconstructor.complete_est_q(),
        task__taskpool_id=taskpool.pk,
    ))
    est_count = len(ests)

    ests_dict = defaultdict(list)
    for est in ests:
        ests_dict[est.task_id].append(est)
    filtered_ests = []
    random.seed(42)
    for est in ests_dict.values():
        if len(est) > taskpool.overlap:
            random.shuffle(est)
        filtered_ests += est[:taskpool.overlap]
    ests = filtered_ests

    (integral_data, task_count,
     tasks_json, has_line_info) = estaggregator.integral_data(ests)
    user_qs = User.objects.filter(
        qconstructor.complete_est_q(prefix='estimation__'),
        estimation__task__taskpool_id=taskpool_id
    ).distinct()
    if criterion:
        crit_val = rawsql.get_crit_expr(criterion)
        value = Estimation.VALUE_NAMES_DICT.keys()[
            Estimation.VALUE_NAMES_DICT.values().index(value_name)
        ]
        user_qs = user_qs.extra(
            select={'crit_val': crit_val},
            where=['%s = %d' % (crit_val, value)]
        )
    user_pks = user_qs.values_list('yandex_uid', flat=True)
    return render(request, 'core/statistics.html', {
        'taskpool': taskpool,
        'filter_crit': criterion, 'filter_val': value_name,
        'est_count': est_count, 'unique_ests_count': len(ests),
        'task_count': task_count, 'integral_data': integral_data,
        'user_pks': user_pks, 'tasks_json': tasks_json,
        'crit_names': ests[0].get_criterions_names() if est_count > 0 else Estimation.Criterion.NAMES_MCR,
        'no_line_info': not has_line_info})


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
def user_statistics(request, taskpool_id, criterion, value_name, yandex_uid):
#This works now only for multicriterial estimations
    try:
        tasks = json.loads(request.POST.get('tasks_json'))
        different = tasks['different']
        opposite = tasks['opposite']
        for i in range(len(Estimation.Criterion.NAMES_MCR)):
            if filter(lambda x: not isinstance(x, int),
                      different[i] + opposite[i]):
                raise TypeError
    except (TypeError, ValueError, KeyError, IndexError):
        return HttpResponse('')
    try:
        taskpool = TaskPool.objects.select_related(
            'first_pool',
            'second_pool'
        ).get(
            pk=taskpool_id
        )
    except ObjectDoesNotExist:
        return HttpResponse('')
    ests_qs = Estimation.objects.filter(
        qconstructor.complete_est_q(),
        task__taskpool_id=taskpool_id,
        user_id=yandex_uid
    ).select_related(
        'user',
        'task',
        'task__first_snippet'
    )
    if request.yauser.core_user.role != User.Role.ANALYST:
        ests_qs = ests_qs.select_related(
            'user',
            'task',
            'task__first_snippet',
            'correction',
            'correction__aadmin_est'
        )
    if criterion:
        crit_val = rawsql.get_crit_expr(criterion)
        value = Estimation.VALUE_NAMES_DICT.keys()[
            Estimation.VALUE_NAMES_DICT.values().index(value_name)
        ]
        ests_qs = ests_qs.extra(
            select={'crit_val': crit_val},
            where=['%s = %d' % (crit_val, value)]
        )
    user_aggregated = estaggregator.user_aggregated_counts(ests_qs, different,
                                                           opposite)
    return render(request, 'core/user_statistics.html', {
        'taskpool': taskpool, 'user_aggregated': user_aggregated
    })


@permission_required(GROUP_DV)
def get_raw_file(request, storage_id):
    response = HttpResponse(Storage.get_raw_file(storage_id),
                            content_type='text/plain; charset=utf-8')
    response['Content-Disposition'] = 'attachment; filename=%s' % storage_id
    return response


@permission_required(GROUP_DV)
def waiting_users(request, page):
    if request.method == 'POST':
        for user in User.objects.filter(
            status=User.Status.WAIT_APPROVE,
            pk__in=request.POST.getlist('uids')
        ):
            is_ang = request.POST.get('is_ang_%s' % user.yandex_uid) == 'on'
            user.status = User.Status.ANG if is_ang else User.Status.ACTIVE
            user.save()
            SteamLogger.info(
                '%(granter)s granted permissions to %(user)s'
                ' with [%(ang)s] ang status',
                type='USER_MANAGEMENT_EVENT',
                granter=request.yauser.core_user.login,
                user=user.login, ang=is_ang
            )
            mailsender.send_letter_on_grant(request, user)
        return redirect('core:waiting_users', page)
    waiting_users_qs = User.objects.filter(
        status=User.Status.WAIT_APPROVE
    )
    cur_page = int(page)
    elements_on_page = 30
    waiting_users = waiting_users_qs[
        cur_page * elements_on_page:
        (cur_page + 1) * elements_on_page
    ]
    pages = {'cur_page': cur_page,
             'obj_count': waiting_users_qs.count(),
             'elements_on_page': elements_on_page}
    return render(request, 'core/waiting_users.html', {
        'waiting_users': waiting_users,
        'pages': pages
    })


@permission_required(GROUP_DV)
def roles(request, page):
    search_request = request.GET.get('search_request', '')

    error = request.GET.get('error')
    users_qs = User.objects.filter(
        login__icontains=search_request,
    ).order_by('login')
    cur_page = int(page)
    elements_on_page = 30
    users = users_qs[
        cur_page * elements_on_page:
        (cur_page + 1) * elements_on_page
    ]
    pages = {'cur_page': cur_page,
             'obj_count': users_qs.count(),
             'elements_on_page': elements_on_page,
             'query_string': request.META['QUERY_STRING']}
    return render(request, 'core/roles.html', {
        'users': users, 'pages': pages, 'error': error,
        'search_request': search_request
    })


@permission_required(GROUP_DV)
def raw_files(request, page):
    querybins = {
        qb.storage_id:
        {'type': 'Query bin', 'info': qb}
        for qb in QueryBin.objects.all() if qb.storage_id
    }
    reports = {
        report.storage_id:
        {'type': 'ANG report', 'info': report}
        for report in ANGReport.objects.all().order_by('-status')
        if report.storage_id
    }
    raw_files = sorted(
        reports.items() + querybins.items(),
        key=lambda x: x[1]['info'].time if x[1]['type'] == 'ANG report'
                                        else x[1]['info'].upload_time,
        reverse=True
    )
    cur_page = int(page)
    elements_on_page = 30
    pages = {'cur_page': cur_page,
             'obj_count': len(raw_files),
             'elements_on_page': elements_on_page}
    return render(request, 'core/raw_files.html', {
        'pages': pages, 'raw_files': raw_files[
            cur_page * elements_on_page:
            (cur_page + 1) * elements_on_page
        ]
    })


def yauth(request):
    redir_url = urllib.unquote(request.GET.get('retpath', '/'))
    # check if this url is ours to eliminate open redirect attack
    try:
        resolve(redir_url)
    except Http404:
        redir_url = '/'
    if not USER_AUTH_REQUIRED or hasattr(request, 'bb_response'):
        return redirect(redir_url)
    passport_url = get_passport_url('create', request=request) + urllib.quote(
        ''.join(('https://', request.get_host(), redir_url))
    )
    return render(request, 'core/yauth.html',
                  {'passport_url': passport_url})


@permission_required(GROUP_ALL)
def user_info(request, login):
    user = get_object_or_404(User, login=login)
    cur_user = request.yauser.core_user
    if cur_user.role == User.Role.ASSESSOR and cur_user != user:
        raise PermissionDenied

    custom_ests = Estimation.objects.filter(
        user_id=user.pk,
    ).values(
        'status',
        'taskpack_id',
        'create_time',
        'taskpack__status',
        'taskpack__last_update',
        'task__status',
        'task__taskpool_id',
        'task__taskpool__title',
        'task__taskpool__deadline',
        'task__taskpool__kind_pool',
    ).annotate(
        status_count=Count('status')
    )

    inspection_est_count = 0
    taskpools_info = {}
    taskpack_statuses = dict(TaskPack.Status.CHOICES)
    ang_period_start = timeprocessor.ang_period_start()
    complete_in_period = 0
    for est in custom_ests:
        pool_id = est['task__taskpool_id']
        if pool_id not in taskpools_info:
            # only one pack for each taskpool
            taskpools_info[pool_id] = {
                'pool_title': est['task__taskpool__title'],
                'kind_pool': est['task__taskpool__kind_pool'],
                'pool_deadline': est['task__taskpool__deadline'],
                'pack_id': est['taskpack_id'],
                'pack_status': est['taskpack__status'],
                'pack_status_str': taskpack_statuses.get(
                    est['taskpack__status']
                ),
                'last_update': est['taskpack__last_update'],
                'complete': 0,
                'rejected': 0,
                'skipped': 0,
                'assigned': 0,
                'total': 0,
                'changed': 0,
                'linked': 0,
            }
        status_count = est['status_count']
        if est['status'] == Estimation.Status.COMPLETE:
            taskpools_info[pool_id]['complete'] += status_count
            taskpools_info[pool_id]['total'] += status_count
            if est['create_time'] >= ang_period_start:
                complete_in_period += status_count
        elif est['status'] == Estimation.Status.REJECTED:
            taskpools_info[pool_id]['rejected'] += status_count
            taskpools_info[pool_id]['total'] += status_count
        elif est['status'] == Estimation.Status.SKIPPED:
            taskpools_info[pool_id]['skipped'] += status_count
        elif est['status'] == Estimation.Status.ASSIGNED:
            taskpools_info[pool_id]['assigned'] += status_count

        if (
            est['task__status'] != Task.Status.REGULAR and
            est['status'] != Estimation.Status.REJECTED and
            est['create_time'] >= ang_period_start
        ):
            inspection_est_count += status_count

    assessor_precisions = []
    if user.role == User.Role.ASSESSOR:
        est_vals = Estimation.objects.filter(
            status=Estimation.Status.COMPLETE,
            user_id=user.pk,
        ).values(
            'json_value',
            'task__taskpool_id',
            'task__status',
            'correction__errors',
            'task__taskpool__kind_pool',
        )
        assessor_precisions = estaggregator.precisions(est_vals)
        for est in est_vals:
            pool_id = est['task__taskpool_id']
            try:
                val = json.loads(est['json_value'], encoding='utf-8')
                if pool_id not in taskpools_info:
                    continue
                taskpools_info[pool_id]['changed'] += int(val.get('changed',
                                                                  False))
                taskpools_info[pool_id]['linked'] += int(val.get('linked',
                                                                 '0'))
            except (TypeError, ValueError):
                continue

    taskpools_info = sorted(taskpools_info.iteritems(),
                            key=lambda x: (x[1]['last_update'],
                                           x[1]['pool_title']),
                            reverse=True)
    complete_est_count = sum(
        pool_info['total'] for pool_id, pool_info in taskpools_info
        if pool_info['pack_status'] in (None, TaskPack.Status.FINISHED)
    )

    return render(request, 'core/user_info.html', {
        'user': user,
        'taskpools_info': taskpools_info,
        'complete_est_count': complete_est_count,
        'inspection_est_count': inspection_est_count,
        'needed_inspection_count': taskdealer.get_t_testing(
            complete_in_period,
            inspection_est_count),
        'assessor_precisions': assessor_precisions,
        'PACK_STATUSES': TaskPack.Status,
        'error': request.GET.get('error')})


@permission_required(GROUP_NO_AS)
def clear_estimations(request):
    Estimation.objects.filter(
        user_id=request.yauser.core_user.pk,
        status__in=(Estimation.Status.ASSIGNED,
                    Estimation.Status.SKIPPED,
                    Estimation.Status.TIMEOUT),
    ).delete()
    return redirect('core:user_info', request.yauser.core_user.login)


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
def assign_pack_back(request, pack_id):
    pack = get_object_or_404(
        TaskPack.objects.select_related(
            'user',
            'taskpool'
        ).exclude(
            taskpool__status=TaskPool.Status.DISABLED
        ),
        pk=pack_id,
        status=TaskPack.Status.TIMEOUT,
    )

    action = 'start'
    if request.method == 'POST':
        form = TaskPoolForm(request.POST, instance=pack.taskpool,
                            action=action)
        if form.is_valid():
            form.save()
            Notification.objects.filter(
                kind=Notification.Kind.DEADLINE,
                taskpack__taskpool__deadline__gt=timezone.now() + timedelta(1)
            ).delete()
            pack.status = TaskPack.Status.ACTIVE
            pack.save()
            return HttpResponse('')
        else:
            response_function = HttpResponseBadRequest
    else:
        form = TaskPoolForm(instance=pack.taskpool, action=action)
        response_function = HttpResponse
    popup_info_struct = {
        'handler_url': reverse('core:assign_pack_back',
                               kwargs={'pack_id': pack_id}),
        'header': ugettext_lazy('Assign pack back'),
        'submit_button': ugettext_lazy('Renew taskpool deadline'),
        'autoopen': True,
        'hide_button': True
    }
    tpl = template.loader.get_template('core/default_popup_form.html')
    return response_function(tpl.render(template.RequestContext(
        request, {'form': form,
                  'popup_info_struct': popup_info_struct})))


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
def take_task_by_as(request, assessor_id):
    assessor = get_object_or_404(User, pk=assessor_id,
                                 role=User.Role.ASSESSOR)
    redir = redirect('core:user_info', assessor.login)
    if not taskdealer.assign_task_by_assessor(request.yauser.core_user,
                                              assessor_id):
        redir['Location'] += '?error=noests'
    return redir


@permission_required(GROUP_ALL)
def notifications(request):
    notifs = Notification.objects.filter(
        user=request.yauser.core_user
    )[:1]
    if notifs:
        return render(request, 'core/notifications/container.html',
                      {'notif': notifs[0]})
    else:
        return HttpResponse('')


@permission_required(GROUP_ALL)
def close_notification(request, notif_id):
    Notification.objects.filter(
        pk=notif_id,
        user=request.yauser.core_user
    ).delete()
    return HttpResponse(notif_id)


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
def est_statistics(request, page):
    qs = request.META['QUERY_STRING']
    if qs:
        form = StatisticsForm(request.GET)
        if not form.is_valid():
            return render(request, 'core/est_statistics.html', {
                'form': form, 'est_infos': (), 'tab': 'count'
            })
        (start_date, end_date) = timeprocessor.stat_dates(form)
    else:
        (start_date, end_date, form) = timeprocessor.default_stat_dates()
    est_infos_qs = Estimation.objects.filter(
        qconstructor.complete_est_q(),
        task__taskpool__kind_pool=TaskPool.TypePool.MULTI_CRITERIAL,
        complete_time__gte=start_date,
        complete_time__lt=end_date
    )
    country = request.GET.get('country', 'All')
    if country != 'All':
        est_infos_qs = est_infos_qs.filter(user__language=country)
    est_infos = map(lambda x: timezone.localtime(x).date(),
                    est_infos_qs.values_list('complete_time', flat=True))
    est_infos = sorted(Counter(est_infos).items(), reverse=True)
    total_count = sum([info[1] for info in est_infos])
    cur_page = int(page)
    elements_on_page = 30
    pages = {'cur_page': cur_page,
             'obj_count': len(est_infos),
             'elements_on_page': elements_on_page,
             'query_string': qs}
    est_infos = est_infos[cur_page * elements_on_page:
                          (cur_page + 1) * elements_on_page]
    return render(request, 'core/est_statistics.html', {
        'form': form, 'est_infos': est_infos, 'total_count': total_count,
        'country': country, 'pages': pages, 'tab': 'count', 'qs': qs,
    })


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
def time_statistics(request):
    qs = request.META['QUERY_STRING']
    if qs:
        form = StatisticsForm(request.GET)
        if not form.is_valid():
            return render(request, 'core/est_statistics.html', {
                'form': form, 'est_infos': (), 'tab': 'time'
            })
        (start_date, end_date) = timeprocessor.stat_dates(form)
    else:
        (start_date, end_date, form) = timeprocessor.default_stat_dates()
    est_infos_qs = Estimation.objects.filter(
        qconstructor.complete_est_q(),
        task__taskpool__kind_pool=TaskPool.TypePool.MULTI_CRITERIAL,
        complete_time__gte=start_date,
        complete_time__lt=end_date
    ).values(
        'rendering_time',
        'i_time',
        'c_time',
        'r_time',
        'json_value',
    )
    country = request.GET.get('country', 'All')
    if country != 'All':
        est_infos_qs = est_infos_qs.filter(user__language=country)
    infos = {
        'all': [],
        'linked': [],
        'unlinked': [],
    }
    for info in est_infos_qs:
        infos['all'].append(info)
        try:
            if json.loads(info['json_value'], encoding='utf-8').get('linked'):
                infos['linked'].append(info)
            else:
                infos['unlinked'].append(info)
        except (TypeError, ValueError):
            infos['unlinked'].append(info)

    stats = {
        'all': [],
        'linked': [],
        'unlinked': [],
    }
    for key in infos:
        times = zip(*[map(lambda x: int(x.total_seconds()), (
            info['r_time'] - info['rendering_time'],
            info['c_time'] - info['r_time'],
            info['i_time'] - info['c_time'],
            info['i_time'] - info['rendering_time'],
        )) for info in infos[key]])
        for times_sample in times:
            stats[key].append(
                mathstat.filtered_sample_parameters(times_sample)
            )
    return render(request, 'core/est_statistics.html', {
        'form': form, 'country': country,
        'stats': stats, 'tab': 'time', 'qs': qs,
    })


@permission_required(GROUP_NO_AS)
@retry()
@transaction.commit_on_success
def assessor_statistics(request):
    qs = request.META['QUERY_STRING']
    if qs:
        form = StatisticsForm(request.GET)
        if not form.is_valid():
            return render(request, 'core/est_statistics.html', {
                'form': form, 'est_infos': (), 'tab': 'assessors'
            })
        (start_date, end_date) = timeprocessor.stat_dates(form)
    else:
        (start_date, end_date, form) = timeprocessor.default_stat_dates()
    common_filter_q = Q(
        task__taskpool__kind_pool=TaskPool.TypePool.MULTI_CRITERIAL,
        complete_time__gte=start_date,
        complete_time__lt=end_date
    )
    taskpack_filter_q = Q(
        last_update__gte=start_date,
        last_update__lt=end_date
    )
    country = request.GET.get('country', 'All')
    if country != 'All':
        common_filter_q &= Q(user__language=country)
        taskpack_filter_q &= Q(user__language=country)
    complete_dicts = Estimation.objects.filter(
        common_filter_q,
        taskpack_id__isnull=False,
        status=Estimation.Status.COMPLETE,
        taskpack__status=TaskPack.Status.FINISHED,
    ).values(
        'status',
        'json_value',
        'user__login',
        'task__status',
        'start_time',
        'complete_time',
        'correction__errors',
        'task__taskpool__kind_pool',
    )
    rejected_counts = Estimation.objects.filter(
        common_filter_q,
        taskpack_id__isnull=False,
        status=Estimation.Status.REJECTED,
        taskpack__status=TaskPack.Status.FINISHED,
    ).values(
        'user__login',
    ).annotate(
        count=Count('user__login')
    )
    skipped_counts = Estimation.objects.filter(
        common_filter_q,
        taskpack_id__isnull=False,
        status=Estimation.Status.SKIPPED,
        taskpack__status=TaskPack.Status.ACTIVE,
    ).values(
        'user__login',
    ).annotate(
        count=Count('user__login')
    )
    timeout_taskpack_counts = TaskPack.objects.filter(
        taskpack_filter_q,
        status=TaskPack.Status.TIMEOUT
    ).values(
        'user__login',
    ).annotate(
        count=Count('user__login')
    )
    aggregated_ests = {}
    for est_dict in complete_dicts:
        login = est_dict['user__login']
        if not aggregated_ests.get(login):
            aggregated_ests[login] = [est_dict]
        else:
            aggregated_ests[login].append(est_dict)
    columns = {}
    samples = {
        'precisions': [],
        'time': [],
        'linked': [],
    }
    medians = {
        'precisions': [1, 1, 1, 1],
        'time': 0,
        'linked': 0,
    }
    sums = {
        'complete': 0,
        'rejected': 0,
        'skipped': 0,
        'timeout_packs': 0,
    }
    for login in aggregated_ests:
        columns[login] = {}
        columns[login]['precisions'] = estaggregator.precisions(
            aggregated_ests[login], flat=True
        )
        columns[login]['time'] = 0
        columns[login]['linked'] = 0
        for est in aggregated_ests[login]:
            columns[login]['time'] += int((est['complete_time'] -
                                           est['start_time']).total_seconds())
            try:
                if json.loads(est['json_value'],
                              encoding='utf-8').get('linked'):
                    columns[login]['linked'] += 1
            except (TypeError, ValueError):
                continue
        complete = len(aggregated_ests[login])
        columns[login]['linked'] = columns[login]['linked'] * 100 / complete
        for key in columns[login]:
            samples[key].append(columns[login][key])
        columns[login]['complete'] = complete
        sums['complete'] += complete

    def add_column(key, counts_qs):
        for value in counts_qs:
            if not columns.get(value['user__login']):
                columns[value['user__login']] = {}
            columns[value['user__login']][key] = value['count']
            sums[key] = value['count']

    add_column('rejected', rejected_counts)
    add_column('skipped', skipped_counts)
    add_column('timeout_packs', timeout_taskpack_counts)
    samples['precisions'] = zip(*samples['precisions'])
    for key in samples:
        if key == 'precisions':
            for idx, sample in enumerate(samples['precisions']):
                if sample:
                    medians['precisions'][idx] = mathstat.median(sample)
        else:
            medians[key] = mathstat.median(samples[key]) or 0
    return render(request, 'core/est_statistics.html', {
        'form': form, 'country': country,
        'tab': 'assessors', 'qs': qs,
        'columns': sorted(columns.items()),
        'medians': medians, 'sums': sums,
        'ext_crit_names': Estimation.Criterion.NAMES_MCR + (
            Estimation.Criterion.OVERALL,
        )
    })


@permission_required(GROUP_ALL)
@transaction.commit_on_success
def page_segmentation(request, est_id):
    # now est_id is fictive: it points to an html number
    est_id = int(est_id)
    if not 1 <= est_id <= len(URLS_LIST):
        return redirect('core:taskpools_default')
    # url /fetch should be proxied in nginx
    # temporary solution for rca_standalone
    url = URLS_LIST[est_id - 1]
    html_url = '/fetch?url=%s&offline=1' % urllib.quote(url)
    if request.yauser.core_user.role == User.Role.ASSESSOR:
        try:
            est = Estimation.objects.get(
                task__request=str(est_id),
                task__taskpool__kind=TaskPool.Type.PAGE_SEGMENTATION,
                user=request.yauser.core_user,
            )
            if est.start_time == ZERO_TIME:
                est.start_time = timezone.now()
                est.save()
        except ObjectDoesNotExist:
            return redirect('core:segmentation_tasks_default')
    else:
        task = Task.objects.get(
            taskpool__kind=TaskPool.Type.PAGE_SEGMENTATION,
            request=str(est_id)
        )
        # TODO: create estimation during task distribution
        try:
            est = Estimation.objects.get(
                task__id=task.id,
                user=request.yauser.core_user,
            )
        except ObjectDoesNotExist:
            cur_time = timezone.now()
            est = Estimation(task_id=task.id, user=request.yauser.core_user,
                             create_time=cur_time, start_time=cur_time,
                             complete_time=ZERO_TIME,
                             status=Estimation.Status.ASSIGNED,
                             json_value={}, taskpack=None)
            est.save()
    if est.status == Estimation.Status.COMPLETE:
        return redirect('core:segmentation_view',
                        est_id, request.yauser.core_user.login)
    if request.method == 'POST':
        form = SegmentationForm(request.POST)
        if form.is_valid():
            if request.yauser.core_user.role == User.Role.ASSESSOR:
                redir = redirect('core:segmentation_tasks_default')
            else:
                redir = redirect('core:segmentation_list_default')
            if request.POST['skipped'] == 'False':
                for stage in Estimation.SegmentationStage.NAMES:
                    est.json_value[stage] = request.POST.getlist(stage)
                est.status = Estimation.Status.COMPLETE
                est.complete_time = timezone.now()
                est_id += 1
                redir = redirect('core:page_segmentation', est_id)
            elif request.POST['skipped'] == 'Rejected':
                est.status = Estimation.Status.REJECTED
                est.complete_time = timezone.now()
                est_id += 1
                redir = redirect('core:page_segmentation', est_id)
            else:
                est.status = Estimation.Status.SKIPPED
            est.comment = request.POST.get('comment')
            est.save()
            return redir
    else:
        form = SegmentationForm(
            initial={'time_elapsed': est.get_elapsed_time(),
                     'comment': est.comment}
        )
    return render(request, 'core/page_segmentation.html',
                  {'html_url': html_url, 'est': est, 'form': form,
                   'stage': 'choice', 'est_id': est_id, 'url': url})


@permission_required(GROUP_NO_AS)
def segmentation_list(request, page):
    est_qs = Estimation.objects.filter(
        status=Estimation.Status.COMPLETE,
        task__taskpool__kind=TaskPool.Type.PAGE_SEGMENTATION
    ).extra(
        select={'docid':
                'CAST(core_task.request AS UNSIGNED)'},
        order_by=('docid', 'user__login')
    )
    cur_page = int(page)
    elements_on_page = 30
    est_infos = est_qs[
        cur_page * elements_on_page:
        (cur_page + 1) * elements_on_page
    ].values(
        'docid', 'user__login'
    )
    pages = {'cur_page': cur_page,
             'obj_count': est_qs.count(),
             'elements_on_page': elements_on_page}
    return render(request, 'core/segmentation_list.html',
                  {'est_infos': est_infos, 'pages': pages})


@permission_required(GROUP_ALL)
@transaction.commit_on_success
def segmentation_view(request, docid, login):
    if request.yauser.core_user.role == User.Role.ASSESSOR:
        redir = redirect('core:segmentation_tasks_default')
        if request.yauser.core_user.login != login:
            return redir
    else:
        redir = redirect('core:segmentation_list_default')
    try:
        est = Estimation.objects.get(
            status=Estimation.Status.COMPLETE,
            task__taskpool__kind=TaskPool.Type.PAGE_SEGMENTATION,
            task__request=docid,
            user__login=login
        )
    except ObjectDoesNotExist:
        return redir
    docid = int(docid)
    if not 1 <= docid <= len(URLS_LIST):
        return redir
    if request.method == 'POST':
        form = SegmentationForm(request.POST)
        if form.is_valid() and request.POST['skipped'] == 'False':
            if est.user != request.yauser.core_user:
                task_id = est.task_id
                json_val = est.json_value
                cur_time = timezone.now()
                try:
                    est = Estimation.objects.exclude(
                        status=Estimation.Status.TIMEOUT,
                    ).get(
                        task_id=task_id,
                        user_id=request.yauser.core_user.yandex_uid
                    )
                    est.json_value['choice'] = json_val['choice']
                    est.status = Estimation.Status.COMPLETE
                    est.complete_time = cur_time
                except ObjectDoesNotExist:
                    est = Estimation(task_id=task_id, user=request.yauser.core_user,
                                     create_time=cur_time, start_time=cur_time,
                                     complete_time=cur_time,
                                     status=Estimation.Status.COMPLETE,
                                     json_value=json_val, taskpack=None)
            est.json_value['merge'] = request.POST.getlist('merge')
            est.comment = request.POST['comment']
            est.save()
            return redirect('core:segmentation_view', docid, request.yauser.core_user.login)
    else:
        form = SegmentationForm(
            initial={'time_elapsed': est.get_elapsed_time(),
                     'comment': est.comment}
        )
    # url /fetch should be proxied in nginx
    # temporary solution for rca_standalone
    url = URLS_LIST[docid - 1]
    html_url = '/fetch?url=%s&offline=1' % urllib.quote(url)
    return render(request, 'core/page_segmentation.html',
                  {'html_url': html_url, 'url': url, 'est_id': docid,
                   'login': login, 'est': est, 'stage': _('view'),
                   'form': form})


@permission_required(GROUP_ALL)
@transaction.commit_on_success
def segmentation_diff(request, docid, login1, login2):
    if login1 == login2:
        return redirect('core:segmentation_view', docid, login1)
    try:
        est1 = Estimation.objects.get(
            status=Estimation.Status.COMPLETE,
            task__taskpool__kind=TaskPool.Type.PAGE_SEGMENTATION,
            task__request=docid,
            user__login=login1
        )
        est2 = Estimation.objects.get(
            status=Estimation.Status.COMPLETE,
            task__taskpool__kind=TaskPool.Type.PAGE_SEGMENTATION,
            task__request=docid,
            user__login=login2
        )
    except ObjectDoesNotExist:
        return redirect('core:segmentation_list_default')
    docid = int(docid)
    if not 1 <= docid <= len(URLS_LIST):
        return redirect('core:segmentation_list_default')
    # !! no validation here
    if request.method == 'POST':
        fixes = request.POST.getlist('fixes')
        est1.status = Estimation.Status.TIMEOUT
        est2.status = Estimation.Status.TIMEOUT
        est1.save()
        est2.save()
        est1.pk = None
        est2.pk = None
        est1.status = Estimation.Status.COMPLETE
        est2.status = Estimation.Status.COMPLETE
        for fix in fixes:
            spl = fix.split(',')
            est1.json_value['merge'][int(spl[0])] = \
                est1.json_value['merge'][int(spl[0])][:-3] + spl[2]
            est2.json_value['merge'][int(spl[1])] = \
                est2.json_value['merge'][int(spl[1])][:-3] + spl[2]
        est1.save()
        est2.save()
        return redirect('core:segmentation_diff', docid, login1, login2)
    # url /fetch should be proxied in nginx
    # temporary solution for rca_standalone
    url = URLS_LIST[docid - 1]
    html_url = '/fetch?url=%s&offline=1' % urllib.quote(url)
    return render(request, 'core/page_segmentation.html',
                  {'html_url': html_url, 'url': url,
                   'est1': est1, 'est2': est2, 'stage': _('diff'),
                   'docid': docid, 'login1': login1, 'login2': login2})


@permission_required(GROUP_ALL)
def segmentation_delete(request, docid, login):
    Estimation.objects.filter(
        task__taskpool__kind=TaskPool.Type.PAGE_SEGMENTATION,
        task__request=docid,
        user__login=login
    ).exclude(
        status=Estimation.Status.TIMEOUT,
    ).update(
        status=Estimation.Status.ASSIGNED,
        complete_time=ZERO_TIME,
        json_value={},
    )
    return redirect(request.META.get('HTTP_REFERER',
                                     'core:segmentation_list_default'))


@permission_required(GROUP_AS)
@transaction.commit_on_success
def segmentation_tasks(request, page):
    elements_on_page = 30
    cur_page = int(page)
    try:
        pack = TaskPack.objects.get(
            user=request.yauser.core_user,
            taskpool__kind=TaskPool.Type.PAGE_SEGMENTATION,
        )
        ests_qs = Estimation.objects.filter(
            taskpack_id=pack.pk,
            task__taskpool__kind=TaskPool.Type.PAGE_SEGMENTATION
        ).extra(
            select={'docid':
                    'CAST(core_task.request AS UNSIGNED)'},
            order_by=('docid', 'status'),
        )
        ests = ests_qs[
            cur_page * elements_on_page:
            (cur_page + 1) * elements_on_page
        ].values(
            'docid',
            'status',
        )
    except ObjectDoesNotExist:
        pack = None
        ests = []
    print(ests)
    elements_on_page = 30
    cur_page = int(page)
    pages = {'cur_page': cur_page,
             'obj_count': (ests_qs.count() if pack else 0),
             'elements_on_page': elements_on_page}
    return render(request, 'core/segmentation_tasks.html',
                  {'pack': pack, 'ests': ests, 'pages': pages})


@permission_required(GROUP_AS)
@transaction.commit_on_success
def segmentation_take(request):
    tp = TaskPool.objects.get(
        kind=TaskPool.Type.PAGE_SEGMENTATION
    )
    try:
        pack = TaskPack.objects.get(
            user=request.yauser.core_user,
            taskpool_id=tp.pk
        )
        return redirect('core:segmentation_tasks_default')
    except ObjectDoesNotExist:
        pass
    tasks = rawsql.segmentation_tasks_to_take()
    cur_time = timezone.now()
    pack = TaskPack(taskpool=tp, user=request.yauser.core_user,
                    last_update=cur_time, status=TaskPack.Status.ACTIVE)
    pack.save()
    ests = [Estimation(
        task_id=task.id, user=request.yauser.core_user,
        create_time=cur_time, start_time=ZERO_TIME,
        complete_time=ZERO_TIME,
        status=Estimation.Status.ASSIGNED,
        json_value={}, taskpack_id=pack.pk
    ) for task in tasks]
    Estimation.objects.bulk_create(ests)
    return redirect('core:segmentation_tasks_default')
