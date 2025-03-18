# -*- coding: utf-8 -*-

from django.db.models import Q


def complete_est_q(prefix=''):
    kwargs_list = recode_kwargs(prefix, [
        {'status': 'C'},            # Estimation.Status.COMPLETE
        {'taskpack_id__isnull': True},
        {'taskpack__status': 'F'}   # TaskPack.Status.FINISHED
    ])
    return Q(
        **kwargs_list[0]
    ) & (
        Q(
            **kwargs_list[1]
        ) | Q(
            **kwargs_list[2]
        )
    )


def recode_kwargs(prefix, kwargs_list):
    for i in range(len(kwargs_list)):
        kwargs_list[i] = {''.join((prefix, key)): kwargs_list[i][key]
                          for key in kwargs_list[i]}
    return kwargs_list
