# -*- coding: utf-8 -*-

from django.db import connection, transaction

from core.models import Task, TaskPool


def get_tasks_range(taskpool_id, offset, limit, search_request, status, order):
    '''
        Returns tasks with completed estimations amount by taskpool.
    '''
    search_condition = (
        'AND LOCATE(%(search_request)s, ct.request) > 0' if search_request
        else ''
    )
    status_condition = (
        'AND ct.status in (\'I\', \'C\')' if status == 'inspection' else ''
    )
    order_by = (
        'ORDER BY MD5(CONCAT(ct.first_snippet_id, ct.second_snippet_id))'
        if order == 'random'
        else ''
    )
    query = '''
        SELECT
            ct.id,
            ct.taskpool_id,
            ct.first_snippet_id,
            ct.second_snippet_id,
            ct.request,
            ct.region,
            COUNT(ce.id) AS completed,
            MAX(ce.id) AS complete_est
        FROM
            core_task AS ct
            LEFT JOIN
            core_estimation AS ce ON
                ct.id = ce.task_id AND
                ce.status = 'C'
            LEFT JOIN
            core_taskpack AS ctp ON
                ce.taskpack_id = ctp.id
        WHERE
            ct.taskpool_id = %%(taskpool_id)s AND (
                ce.taskpack_id IS NULL OR
                ctp.status = 'F'
            )
            %(search_condition)s
            %(status_condition)s
        GROUP BY
            ct.id,
            ct.taskpool_id,
            ct.first_snippet_id,
            ct.second_snippet_id,
            ct.request,
            ct.region
        %(order_by)s
        LIMIT %%(limit)s OFFSET %%(offset)s
    ''' % {'search_condition': search_condition,
            'status_condition': status_condition,
            'order_by': order_by}
    return Task.objects.raw(query,
                            {'taskpool_id': taskpool_id,
                             'search_request': search_request,
                             'limit': limit, 'offset': offset})


def get_tasks_for_estimations(taskpool, excluded_tasks):
    '''
        returns tasks from <taskpool> with minimal overlap
        that can be assigned to assessor (not exceeding overlap)
        excluding <excluded_tasks>
    '''
    if excluded_tasks:
        excluded_tasks_query = 'AND ct.id NOT IN (%s)' % (
            ', '.join([str(t.id) for t in excluded_tasks])
        )
    else:
        excluded_tasks_query = ''

    count = taskpool.pack_size - len(excluded_tasks)
    return Task.objects.raw(
        '''
            SELECT
                ct.id,
                COUNT(ce.id) AS amount
            FROM
                core_task AS ct LEFT JOIN
                core_estimation AS ce ON
                    ct.id = ce.task_id AND
                    ce.status IN ('C', 'A', 'S')
            WHERE
                ct.taskpool_id = %%s
                %(excluded_tasks_query)s
            GROUP BY
                ct.id
            HAVING
                amount < %%s
            ORDER BY
                amount
            LIMIT
                %%s
        ''' % {'excluded_tasks_query': excluded_tasks_query},
        (taskpool.pk, taskpool.overlap, count)
    )


def get_inspection_tasks_for_estimations(taskpool, count):
    '''
        returns <count> of inspection tasks
        with minimal overlap from <taskpool>
    '''
    return Task.objects.raw(
        '''
            SELECT
                ct.id,
                COUNT(ce.id) AS amount
            FROM
                core_task AS ct LEFT JOIN
                core_estimation AS ce ON
                    ct.id = ce.task_id AND
                    ce.status IN ('C', 'A', 'S')
            WHERE
                ct.taskpool_id = %s AND
                ct.status = 'I'
            GROUP BY
                ct.id
            ORDER BY
                amount
            LIMIT
                %s
        ''',
        (taskpool.pk, count)
    )


# this function is used for taskpool overlapping, not finishing.
# so we don't look at packs
def taskpools_not_to_finish():
    '''
        Selects all active taskpools except those
        that have tasks with not enough completed estimations to reach overlap
    '''

    return [taskpool.pk for taskpool in TaskPool.objects.raw('''
        SELECT
            p.id
        FROM
            core_taskpool AS p JOIN (
                SELECT
                    t.id,
                    t.taskpool_id,
                    COUNT(e.id) AS complete_ests
                FROM
                    core_task AS t LEFT JOIN
                    core_estimation AS e ON
                        e.task_id = t.id AND
                        e.status = 'C'
                GROUP BY
                    t.id,
                    t.taskpool_id
            ) AS te ON
                p.status = 'A' AND
                te.taskpool_id = p.id AND
                te.complete_ests < p.overlap
        GROUP BY
            p.id
    ''')]


def get_crit_expr(criterion):
    return '''
        CAST(TRIM(',' FROM SUBSTRING(
            TRIM('}' FROM core_estimation.json_value),
            LOCATE('%s', core_estimation.json_value) + %d,
            2
        )) AS SIGNED)
    ''' % (criterion, len(criterion) + 3)  # 3 == len('": ')


def make_correction_relations(aadmin_est, est_ids, batch_time):
    cursor = connection.cursor()
    cursor.execute(
        '''
            UPDATE
                core_estimation AS ce
            SET
                correction_id = (
                    SELECT
                        id
                    FROM
                        core_correction
                    WHERE
                        time = %%s AND
                        assessor_est_id = ce.id AND
                        aadmin_est_id = %%s
                )
            WHERE
                id IN (%s)
        ''' % ', '.join([str(est_id) for est_id in est_ids]),
        (batch_time, aadmin_est.id)
    )
    transaction.commit_unless_managed()


def segmentation_tasks_to_take():
    start_task = Task.objects.raw(
        '''
            SELECT
                ct.id,
                CAST(ct.request AS UNSIGNED) AS docid,
                count(ce.id) AS est_count
            FROM
                core_task AS ct JOIN
                core_taskpool AS ctp ON
                    ct.taskpool_id = ctp.id AND
                    ctp.kind = 'SGM'
                LEFT JOIN
                    core_estimation AS ce ON
                        ce.task_id = ct.id AND
                        ce.taskpack_id IS NOT NULL
            GROUP BY
                ct.id,
                docid
            HAVING
                est_count < 2 AND
                docid %% 200 = 0
            ORDER BY
                docid
            LIMIT
                1
        '''
    )
    if start_task:
        return Task.objects.filter(
            taskpool__kind=TaskPool.Type.PAGE_SEGMENTATION,
        ).extra(
            select={'docid':
                    'CAST(request AS UNSIGNED)'},
            where=['CAST(request AS UNSIGNED) BETWEEN %d AND %d' % (
                start_task[0].docid,
                start_task[0].docid + 199
            )]
        )
    else:
        return []
