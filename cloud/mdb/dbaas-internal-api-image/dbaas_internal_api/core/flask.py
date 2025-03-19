# -*- coding: utf-8 -*-
"""
Extensions to flask and flask_restful.
"""

import flask_restful
from werkzeug.exceptions import HTTPException

from .exceptions import DbaasHttpError


class Api(flask_restful.Api):
    """
    Specialization of flask_restful.Api with custom error formatting.
    """

    def __init__(self, **kwargs) -> None:
        super().__init__(**kwargs)

    def handle_error(self, e: Exception):  # pylint: disable=invalid-name
        return super().handle_error(self._format_error(e))

    @classmethod
    def _format_error(cls, exc: Exception) -> DbaasHttpError:
        if isinstance(exc, DbaasHttpError):
            return exc

        if isinstance(exc, HTTPException):
            return DbaasHttpError(cls._format_message(exc), exc.code)

        return DbaasHttpError('The service is currently unavailable')

    @staticmethod
    def _format_message(exc: HTTPException) -> str:
        data = getattr(exc, 'data', {})
        messages = data.get('messages')
        if not messages:
            return data.get('message', exc.description)

        message_list = []

        def _flatten_messages(messages, field=None):
            if isinstance(messages, dict):
                for name, msg in messages.items():
                    if name == '_schema':
                        msg_field = field
                    elif field:
                        msg_field = '{0}.{1}'.format(field, name)
                    else:
                        msg_field = name
                    _flatten_messages(msg, msg_field)

            elif isinstance(messages, list):
                for msg in messages:
                    _flatten_messages(msg, field)

            else:
                prefix = field + ': ' if field else ''
                message_list.append((prefix, messages))

        _flatten_messages(messages)

        return 'The request is invalid.\n{0}'.format(
            '\n'.join(prefix + str(msg) for prefix, msg in sorted(message_list))
        )
