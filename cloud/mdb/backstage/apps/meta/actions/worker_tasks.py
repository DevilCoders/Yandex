import logging

import django.db

import cloud.mdb.backstage.apps.main.models as main_models
import cloud.mdb.backstage.lib.helpers as mod_helpers

import cloud.mdb.backstage.apps.meta.models as mod_models


logger = logging.getLogger('backstage.meta.actions.worker_tasks')


def process_action(
    objects,
    action,
    action_params,
    username,
    client_ip,
    request_id,
):
    results = []
    for obj in objects:
        result = process_task_action(
            action=action,
            obj=obj,
            username=username,
            client_ip=client_ip,
            request_id=request_id,
            comment=action_params.get('comment'),
        )
        results.append(result)
    return results


def process_task_action(
    action,
    obj,
    username,
    client_ip,
    request_id,
    comment,
):
    try:
        result, err = obj.action_ability(action)
        if not result:
            return mod_helpers.ActionResult(obj=obj, error=f'failed to process action: {err}')

        if not comment:
            return mod_helpers.ActionResult(obj=obj, error='comment should not be empty')

        with django.db.transaction.atomic(using='meta_db'):
            with django.db.transaction.atomic():
                if action == mod_models.WorkerTaskAction.CANCEL[0]:
                    obj.cancel(username, comment)
                    message = 'task has been cancelled'
                elif action == mod_models.WorkerTaskAction.RESTART[0]:
                    obj.restart(username, comment)
                    message = 'task has been restarted'
                elif action == mod_models.WorkerTaskAction.REJECT[0]:
                    obj.reject(username, comment)
                    message = 'task has been rejected'
                elif action == mod_models.WorkerTaskAction.FINISH[0]:
                    obj.finish(username, comment)
                    message = 'task has been finished'
                else:
                    raise ValueError('Should not happens')
                main_models.AuditLog.write_action(
                    message=message,
                    username=username,
                    user_address=client_ip,
                    user_comment=comment,
                    request_id=request_id,
                    obj=obj,
                    action=action,
                )
                return mod_helpers.ActionResult(obj=obj, message=message)
    except Exception as err:
        error=f'failed to process action: {err}'
        logger.exception(error)
        return mod_helpers.ActionResult(obj=obj, error=error)
