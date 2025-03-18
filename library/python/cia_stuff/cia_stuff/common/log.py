# coding: utf-8

from __future__ import unicode_literals

import logging

from ylog import context


def enable_console_for_logger(name, level=logging.DEBUG):
    logger = logging.getLogger(name)
    handler = logging.StreamHandler()
    handler.setLevel(level)
    logger.addHandler(handler)
    logger.setLevel(level)


class ContextLoggingMiddleware(object):

    def process_request(self, request):
        yauser = getattr(request, 'yauser')
        for key, value in {
            'login': yauser and getattr(yauser, 'login', None) or '(not logged in)',
            'req_id': id(request),
        }.items():
            context.put_to_context(key, value)

    def process_response(self, request, response):
        context.pop_from_context('login')
        context.pop_from_context('req_id')
        return response


class SuppressDeprecated(logging.Filter):
    # TODO: make configurable
    WARNINGS_TO_SUPPRESS = [
        'RemovedInDjango18Warning',
        'RemovedInDjango19Warning',
    ]

    def filter(self, record):
        # Return false to suppress message.
        return not any([
            warn in record.getMessage() for warn in self.WARNINGS_TO_SUPPRESS
        ])
