# coding: utf-8
from __future__ import unicode_literals

import json

from ids.exceptions import BackendError

from .connector import OrangeConnector as Connector


def ping(oauth_token, user_agent):
    """
    Проверка связи
    https://wiki.yandex-team.ru/Intranet/orange/api/ping
    """
    connector = Connector(oauth_token=oauth_token, user_agent=user_agent)
    response = connector.get(resource='ping')

    try:
        return response.json()
    except ValueError as error:
        raise BackendError(str(error), response=response)


def push_notification(oauth_token, user_agent, description, target_uids, **params):
    """
    Создание уведомления
    https://wiki.yandex-team.ru/Intranet/orange/api/push-notification

    @param description: dict (см. документацию)
    @param target_uids: последовательность паспортных уидов
    """
    connector = Connector(oauth_token=oauth_token, user_agent=user_agent)
    data = {
        'description': description,
        'targetUids': target_uids
    }
    response = connector.post(
        resource='notifications_push',
        data=json.dumps(data),
        **params
    )

    try:
        return response.json()
    except ValueError as error:
        raise BackendError(error.message, response=response)


def push_callback_notification(oauth_token, user_agent, description,
                               target_uids, request_type, accept_type,
                               comment_required=False, **params):
    """
    Создание запроса на согласование
    https://wiki.yandex-team.ru/Intranet/orange/api/push-callback-notification

    @param description: dict (см. документацию)
    @param target_uids: последовательность паспортных уидов
    @param request_type: тип операции согласования (будет использоваться
    в обратном вызове)
    @param accept_type: ANY/ALL (см. документацию)
    @param comment_required: требовать комментарий при подтверждении/отклонении
    """

    assert accept_type in ('ANY', 'ALL'), 'Invalid accept_type'

    connector = Connector(oauth_token=oauth_token, user_agent=user_agent)
    data = {
        'description': description,
        'targetUids': target_uids,
        'acceptType': accept_type,
        'type': request_type,
        'commentRequired': comment_required,
    }
    response = connector.post(
        resource='notifications_push_callback',
        data=json.dumps(data),
        **params
    )

    try:
        return response.json()
    except ValueError as error:
        raise BackendError(error.message, response=response)


def delete_notification(oauth_token, user_agent, id, uid=None, **params):
    """
    Удаление уведомления или запроса согласования с идентификатором {id}
    https://wiki.yandex-team.ru/Intranet/orange/api/delete-notification

    Если передан uid, то нотификация удаляется для пользователя с этим {uid}
    https://wiki.yandex-team.ru/Intranet/orange/api/delete-user-notification
    """
    connector = Connector(oauth_token=oauth_token, user_agent=user_agent)

    url_vars = {'id': id}
    if uid is None:
        resource = 'notifications_delete'
    else:
        url_vars['uid'] = uid
        resource = 'notifications_delete_user'

    try:
        response = connector.delete(
            resource=resource,
            url_vars=url_vars,
            **params
        )
    except BackendError as error:
        if error.status_code == 404:
            response = error.response
        else:
            raise

    if response.content:
        try:
            return response.json()
        except ValueError as error:
            raise BackendError(error.message, response=response)
