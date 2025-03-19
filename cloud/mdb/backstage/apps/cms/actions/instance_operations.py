import logging

import django.db

import cloud.mdb.backstage.apps.main.models as main_models
import cloud.mdb.backstage.lib.helpers as mod_helpers

import cloud.mdb.backstage.apps.cms.models as mod_models


logger = logging.getLogger('backstage.cms.actions.instance_operations')


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
        if action == mod_models.InstanceOperationAction.STATUS_TO_OK[0]:
            results.append(status_to_ok(
                obj=obj,
                username=username,
                comment=action_params.get('comment'),
                client_ip=client_ip,
                request_id=request_id,
            ))
        elif action == mod_models.InstanceOperationAction.STATUS_TO_IN_PROGRESS[0]:
            results.append(status_to_in_progress(
                obj=obj,
                username=username,
                comment=action_params.get('comment'),
                client_ip=client_ip,
                request_id=request_id,
            ))
    return results


def change_status(
    obj,
    action,
    new_status,
    username,
    comment,
    client_ip,
    request_id,
):
    try:
        result, err = obj.action_ability(action)
        if not result:
            return mod_helpers.ActionResult(obj=obj, error=f'failed to process action: {err}')

        with django.db.transaction.atomic(using='cms_db'):
            with django.db.transaction.atomic():
                prev_status = obj.status
                obj.status = new_status
                obj.save()
                message = f'Status has been changed from {prev_status} to {new_status}'
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


def status_to_ok(
    obj,
    username,
    comment,
    client_ip,
    request_id,
):
    return change_status(
        obj=obj,
        action=mod_models.InstanceOperationAction.STATUS_TO_OK[0],
        new_status=mod_models.InstanceOperationStatus.OK_PENDING[0],
        username=username,
        comment=comment,
        client_ip=client_ip,
        request_id=request_id,
    )


def status_to_in_progress(
    obj,
    username,
    comment,
    client_ip,
    request_id,
):
    return change_status(
        obj=obj,
        action=mod_models.InstanceOperationAction.STATUS_TO_IN_PROGRESS[0],
        new_status=mod_models.InstanceOperationStatus.IN_PROGRESS[0],
        username=username,
        comment=comment,
        client_ip=client_ip,
        request_id=request_id,
    )
