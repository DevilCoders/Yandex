# coding: utf-8

from functools import wraps


def wrap_session_send(old_session_send, domain_provider):
    """
    @type old_session_send: callable
    @type domain_provider: RequestsLibDomainProvider
    """

    @wraps(old_session_send)
    def wrapper(self, request, **kwargs):
        try:
            response = old_session_send(self, request, **kwargs)
            return response
        finally:
            domain_provider.set_context(request=request, response=response).make_records()

    return wrapper
