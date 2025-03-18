# -*- coding: utf-8 -*-

import json

from django.core.exceptions import ObjectDoesNotExist

from core.actions import qconstructor
from core.models import Estimation, TaskPool


def export_estimations(taskpool_id):
    ests = Estimation.objects.select_related(
        'task',
        'task__first_snippet',
        'task__second_snippet',
        'user',
        'correction',
        'correction__aadmin_est'
    ).filter(
        qconstructor.complete_est_q(),
        task__taskpool=taskpool_id,
    )
    if not ests:
        return None
    try:
        taskpool = TaskPool.objects.get(pk=taskpool_id)
        taskpool_info = {
            'title': taskpool.title,
            'kind': taskpool.kind,
            'countres': [tpc.country for tpc in taskpool.tpcountry_set.all()],
            'overlap': taskpool.overlap,
            'tasks_amount': taskpool.overlap_count(),
            'tasks_status': taskpool.tasks_status(),
            'tasks_completed': taskpool.completed_count(),
        }
    except ObjectDoesNotExist:
        taskpool_info = None

    ests_json = json.dumps(
        {
            'taskpool': taskpool_info,
            'estimations': [
                {
                    'assessor': est.user.login,
                    'url': est.task.first_snippet.data['url'],
                    'request': est.task.request,
                    'region': est.task.region,
                    'first_snippet': {
                        'title': est.task.first_snippet.data['title'],
                        'headline': est.task.first_snippet.data.get('headline', ''),
                        'text': est.task.first_snippet.data['text'],
                        'headline_src': est.task.first_snippet.data.get('headline_src', ''),
                        'extra': est.task.first_snippet.data.get('extra', {}),
                    },
                    'second_snippet': {
                        'title': est.task.second_snippet.data['title'],
                        'headline': est.task.second_snippet.data.get('headline', ''),
                        'text': est.task.second_snippet.data['text'],
                        'headline_src': est.task.second_snippet.data.get('headline_src', ''),
                        'extra': est.task.second_snippet.data.get('extra', {}),
                    },
                    'values': est.corrected_values(),
                    'changed': est.json_value.get('changed', False)
                }
                for est in ests
            ],
        }, encoding='utf-8', ensure_ascii=False, indent=4)
    return ests_json
