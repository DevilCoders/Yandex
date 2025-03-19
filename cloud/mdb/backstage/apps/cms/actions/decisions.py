import logging

import django.db
import django.utils.timezone as timezone

import cloud.mdb.backstage.apps.main.models as main_models
import cloud.mdb.backstage.lib.helpers as mod_helpers

import cloud.mdb.backstage.apps.cms.models as mod_models


logger = logging.getLogger('backstage.cms.actions.decisions')


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
        if action == mod_models.DecisionAction.APPROVE[0]:
            result = approve(
                obj=obj,
                username=username,
                client_ip=client_ip,
                request_id=request_id,
                comment=action_params.get('comment')
            )
            results.append(result)
        elif action == mod_models.DecisionAction.MARK_AS_DONE[0]:
            result = mark_as_done(
                obj=obj,
                username=username,
                client_ip=client_ip,
                request_id=request_id,
                comment=action_params.get('comment')
            )
            results.append(result)
        elif action == mod_models.DecisionAction.OK_AT_WALLE[0]:
            result = ok_at_walle(
                obj=obj,
                username=username,
                client_ip=client_ip,
                request_id=request_id,
                comment=action_params.get('comment')
            )
            results.append(result)
    return results


def approve(
    obj,
    username,
    client_ip,
    request_id,
    comment,
):
    action = mod_models.DecisionAction.APPROVE[0]
    try:
        result, err = obj.action_ability(action)
        if not result:
            return mod_helpers.ActionResult(obj=obj, error=f'failed to process action: {err}')

        with django.db.transaction.atomic(using='cms_db'):
            with django.db.transaction.atomic():
                prev_status = obj.status
                new_status = mod_models.DecisionStatus.OK[0]
                obj.status = new_status
                obj.request.analysed_by = username
                obj.save()
                obj.request.save()

                message = f'status has been changed from {prev_status} to {new_status}'
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


def mark_as_done(
    obj,
    username,
    client_ip,
    request_id,
    comment,
):
    action = mod_models.DecisionAction.MARK_AS_DONE[0]
    try:
        result, err = obj.action_ability(action)
        if not result:
            return mod_helpers.ActionResult(obj=obj, error=f'failed to process action: {err}')

        with django.db.transaction.atomic(using='cms_db'):
            with django.db.transaction.atomic():
                prev_status = obj.status
                new_status = mod_models.DecisionStatus.CLEANUP[0]
                obj.status = new_status
                obj.save()
                obj.request.save()

                message = f'status has been changed from {prev_status} to {new_status}'
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


def ok_at_walle(
    obj,
    username,
    client_ip,
    request_id,
    comment,
):
    action = mod_models.DecisionAction.OK_AT_WALLE[0]
    try:
        result, err = obj.action_ability(action)
        if not result:
            return mod_helpers.ActionResult(obj=obj, error=f'failed to process action: {err}')

        with django.db.transaction.atomic(using='cms_db'):
            with django.db.transaction.atomic():
                prev_status = obj.status
                new_status = mod_models.DecisionStatus.AT_WALLE[0]
                obj.status = new_status
                obj.request.status = mod_models.RequestStatus.OK[0]
                obj.request.resolved_by = username
                obj.request.resolved_at = timezone.now()
                obj.save()
                obj.request.save()

                message = f'status has been changed from {prev_status} to {new_status}'
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
