# -*- coding: utf-8 -*-

from django.core.exceptions import ObjectDoesNotExist
from django.db.models import Q
from django.shortcuts import redirect
from django.utils import timezone

from core.hard.loghandlers import SteamLogger
from core.models import (Correction, Estimation, Notification, SnippetPool,
                         Task, TaskPack, TaskPool, User)
from core.settings import ZERO_TIME


def get_pool_if_exists(snippet):
    try:
        return SnippetPool.objects.get(pk=snippet.snippet_id.split('_')[0])
    except ObjectDoesNotExist:
        return None


def get_redirect(user, est_id, exp_criterion):
    redir = redirect('core:usertasks_default', tab='current')
    filter_kwargs = {'pk': est_id}
    if user.role == User.Role.ASSESSOR:
        filter_kwargs['user'] = user
    try:
        est = Estimation.objects.select_related(
            'task__taskpool',
            'taskpack',
        ).get(**filter_kwargs)
    except ObjectDoesNotExist:
        redir['Location'] += '?error=inexistent'
    else:
        # if user wants to estimate specific criterion, but
        # he is not an owner of this estimation or
        # estimation is closed or
        # he did not complete previously needed criterions
        # then redirect him to default estimation page

        if exp_criterion and (
            user != est.user or
            est.status not in (Estimation.Status.ASSIGNED, Estimation.Status.SKIPPED) or
            est.get_criterions_names().index(est.get_cur_criterion()) >
            est.get_criterions_names().index(exp_criterion)
        ):
            redir = redirect('core:estimation', est_id)
            redir['Location'] += '?error=criterion'
        if (
            est.task.taskpool.status == TaskPool.Status.DISABLED and
            est.status in (Estimation.Status.ASSIGNED,
                           Estimation.Status.SKIPPED) and
            user.role not in (User.Role.ANALYST,
                              User.Role.DEVELOPER)
        ):
            redir['Location'] += '?error=disabled'
        elif est.is_timeout():
            redir['Location'] += '?error=timeout'
        elif (
            user != est.user and
            (user.role == User.Role.ASSESSOR or
             est.status not in (Estimation.Status.COMPLETE,
                                Estimation.Status.SKIPPED,
                                Estimation.Status.REJECTED))
        ):
            redir['Location'] += '?error=inexistent'
    return redir


# returns snippet rendering kwargs,
#         boolean value telling if to show request,
#         current criterion to render EstimationForm with,
#         previous criterion if assessor may go back (or None),
#         next criterion if assessor may go further (or None)
def criterion_based_values(est, user, exp_criterion):
    cur_criterion = est.get_cur_criterion()
    criterion = exp_criterion or cur_criterion
    snippet_rendering_kwargs = {}
    show_request = True
    next_criterion = None
    prev_criterion = None
    if user == est.user and est.task.taskpool.kind_pool == TaskPool.TypePool.MULTI_CRITERIAL:
        if criterion in (
            Estimation.Criterion.READABILITY,
            Estimation.Criterion.CONTENT_RICHNESS
        ):
            show_request = False
            snippet_rendering_kwargs['b_tags'] = False
            if cur_criterion != criterion:
                next_criterion = est.get_criterions_names()[
                    est.get_criterions_names().index(criterion) - 1
                ]
            if criterion == Estimation.Criterion.READABILITY:
                snippet_rendering_kwargs['url'] = False
            else:
                prev_criterion = Estimation.Criterion.READABILITY
        else:
            prev_criterion = Estimation.Criterion.CONTENT_RICHNESS
    return (snippet_rendering_kwargs, show_request,
            criterion, prev_criterion, next_criterion)


def reopen_inspection_tasks(taskpack):
    cur_time = timezone.now()
    notifications = []
    create_notifications = (taskpack.taskpool.status ==
                            TaskPool.Status.FINISHED)
    for est in Estimation.objects.select_related(
        'task',
    ).filter(
        taskpack_id=taskpack.id,
        task__status=Task.Status.COMPLETE,
        status=Estimation.Status.COMPLETE
    ):
        aadmin_correction = Correction.objects.filter(
            aadmin_est__task_id=est.task_id,
            status__in=(Correction.Status.ACTUAL,
                        Correction.Status.REPORTED)
        ).order_by('-time')[0]
        est.task.status = Task.Status.INSPECTION
        est.task.save()
        if create_notifications:
            aadmin_id = aadmin_correction.aadmin_est.user_id
            notifications.append(
                Notification(user_id=aadmin_id, kind=Notification.Kind.RECHECK,
                             estimation_id=aadmin_correction.aadmin_est_id)
            )
        # save corrections for other users
        old_corrections = Correction.objects.filter(
            aadmin_est_id=aadmin_correction.aadmin_est_id,
            time=aadmin_correction.time,
            status=aadmin_correction.status
        )

        for corr in old_corrections:
            # create new object when save
            corr.pk = None
            corr.status = Correction.Status.SAVED
            corr.time = cur_time
        Correction.objects.bulk_create(old_corrections)
    if create_notifications:
        Notification.objects.bulk_create(notifications)


def finish_estimation(request, est, criterion=None):
    old_status = est.status
    old_criterion = est.get_cur_criterion()
    create_correction = False
    cur_time = timezone.now()
    est.complete_time = cur_time
    skipped = ('False'
               if est.status == Estimation.Status.COMPLETE
               else request.POST['skipped'])
    if skipped == 'Rejected':
        est.status = Estimation.Status.REJECTED
        est.json_value = Estimation.DEFAULT_VALUE
    else:
        est.json_value['linked'] = (est.json_value.get('linked', False) or
                                    ('1' == request.POST.get('linked')))
        if skipped == 'Questioned':
            # don't save value
            est.status = Estimation.Status.SKIPPED
            notifications = [
                Notification(user=aadmin, kind=Notification.Kind.QUESTION,
                             estimation=est)
                for aadmin in User.objects.filter(
                    language=est.user.language,
                    role=User.Role.AADMIN,
                )
            ]
            Notification.objects.bulk_create(notifications)
        else:
            # TODO: can be more than 1 criterion in POST
            est.json_value.update(est.pack_value(request.POST))
            if criterion != None and getattr(
                est, Estimation.TIMES[criterion]
            ) == ZERO_TIME:
                setattr(est, Estimation.TIMES[criterion], cur_time)
            if (
                est.status == Estimation.Status.COMPLETE or
                criterion != old_criterion
            ):
                est.json_value['changed'] = True
                SteamLogger.info(
                    'User %(user)s changed estimation %(est_id)d value',
                    type='ESTIMATION_CHANGE_EVENT',
                    user=est.user.login, est_id=est.pk
                )
            if not est.get_cur_criterion():
                est.status = Estimation.Status.COMPLETE
            if request.yauser.core_user.role == User.Role.AADMIN:
                # AA has just completed this task. We should not send
                # statistics to ANG until he checks this task.
                est.task.status = Task.Status.INSPECTION
                est.task.save()
            elif (
                request.yauser.core_user.role in (User.Role.ANALYST,
                                                  User.Role.DEVELOPER) and
                est.task.status == Task.Status.COMPLETE and
                old_status != Estimation.Status.COMPLETE and
                est.status == Estimation.Status.COMPLETE
            ):
                create_correction = True

    est.comment = request.POST.get('comment', '')
    if old_status == Estimation.Status.SKIPPED and est.status != old_status:
        Notification.objects.filter(
            kind=Notification.Kind.QUESTION,
            estimation=est
        ).delete()
    est.save()
    if (
        request.yauser.core_user.role == User.Role.ASSESSOR and
        est.status in (Estimation.Status.COMPLETE,
                       Estimation.Status.REJECTED)
    ):
        # assessor's estimations have taskpack
        est.taskpack.last_update = cur_time
        if not Estimation.objects.filter(
            taskpack_id=est.taskpack_id,
            status__in=(Estimation.Status.ASSIGNED,
                        Estimation.Status.SKIPPED)
        ).exists():
            # taskpack is over
            est.taskpack.status = TaskPack.Status.FINISHED
            Notification.objects.filter(
                kind=Notification.Kind.DEADLINE,
                taskpack_id=est.taskpack.pk
            ).delete()
            reopen_inspection_tasks(est.taskpack)
        est.taskpack.save()
    if create_correction:
        existing_correction = Correction.objects.filter(
            status__in=(Correction.Status.ACTUAL, Correction.Status.REPORTED),
            aadmin_est__task_id = est.task_id
        ).order_by('-time')[0]
        new_correction = Correction(
            aadmin_est_id=existing_correction.aadmin_est_id,
            assessor_est=est,
            time=existing_correction.time,
            errors=0, comment='',
            status=existing_correction.status
        )
        new_correction.save()
        est.correction_id = new_correction.id
        est.save()


def redirect_after_check(user):
    ests_to_make = Estimation.objects.filter(
        user=user, status=Estimation.Status.ASSIGNED
    ).exclude(
        task__taskpool__status=TaskPool.Status.DISABLED
    )[:1]
    if ests_to_make:
        return redirect('core:estimation', ests_to_make[0].id)
    ests_to_check = Estimation.objects.filter(
        user=user, status=Estimation.Status.COMPLETE,
        task__status=Task.Status.INSPECTION,
        task__taskpool__status=TaskPool.Status.FINISHED
    )[:1]
    if ests_to_check:
        return redirect('core:estimation_check', ests_to_check[0].id)
    return redirect('core:available_usertasks_default')


def redirect_after_estimation(request, est):
    user = request.yauser.core_user
    if (
        request.POST['skipped'] == 'False' and
        est.status != Estimation.Status.COMPLETE
    ):
        return redirect('core:estimation', est.id)
    if (
        user.role == User.Role.AADMIN and
        request.POST['skipped'] == 'False' and
        est.task.status == Task.Status.INSPECTION and
        est.task.taskpool.status == TaskPool.Status.FINISHED
    ):
        return redirect('core:estimation_check', est.id)
    exclude_Q = Q(task__taskpool__status=TaskPool.Status.DISABLED)
    if user.role == User.Role.ASSESSOR:
        exclude_Q |= Q(taskpack__status=TaskPack.Status.TIMEOUT)
    elif user.role in (User.Role.ANALYST, User.Role.DEVELOPER):
        # AN and DV can take disabled ests
        exclude_Q = Q()
    ests_to_make = Estimation.objects.filter(
        user=user, status=Estimation.Status.ASSIGNED
    ).exclude(
        exclude_Q
    ).order_by('task__taskpool__deadline')[:1]
    if ests_to_make:
        return redirect('core:estimation', ests_to_make[0].id)
    else:
        if user.role == User.Role.AADMIN:
            return redirect('core:available_usertasks_default')
    return redirect('core:usertasks_default', tab='current')


def answer_question(request, est):
    est.status = Estimation.Status.ASSIGNED
    est.answer = request.POST['answer']
    est.create_time = timezone.now()
    est.start_time = est.complete_time = ZERO_TIME
    est.save()
    Notification.objects.filter(
        kind=Notification.Kind.QUESTION,
        estimation=est
    ).delete()
