# -*- coding: utf-8 -*-

from django.utils import timezone

from core.models import Estimation, TaskPack


def create_packs(user, est_class=Estimation, pack_class=TaskPack):
    delete_incomplete_ests(user, est_class)
    est_qs = est_class.objects.filter(
        user=user
    )
    taskpool_pks = est_qs.distinct().values_list(
        'task__taskpool_id', flat=True
    )
    pack_creation_time = timezone.now()
    for taskpool_pk in taskpool_pks:
        new_pack = pack_class(user=user, taskpool_id=taskpool_pk,
                              last_update=pack_creation_time,
                              status=TaskPack.Status.FINISHED)
        new_pack.save()
        est_qs.filter(
            task__taskpool_id=taskpool_pk
        ).update(
            taskpack=new_pack
        )


def delete_packs(user, pack_class=TaskPack):
    pack_class.objects.filter(
        user=user
    ).delete()


def delete_incomplete_ests(user, est_class=Estimation):
    est_class.objects.filter(
        user=user
    ).exclude(
        status__in=(Estimation.Status.REJECTED, Estimation.Status.COMPLETE)
    ).delete()
