# coding: utf-8
from __future__ import unicode_literals, absolute_import

import re

from django.http import HttpResponse
from django.conf import settings

from .state import get_state
from .loading import get_alive_middleware_reduce
from .views import state_detail


class AliveMiddleware(object):
    def __init__(self):
        self.ping_re = re.compile(settings.ALIVE_MIDDLEWARE_PING_PATTERN)
        self.status_re = re.compile(settings.ALIVE_MIDDLEWARE_STATE_PATTERN)

    def process_request(self, request):
        if self.ping_re.match(request.path):
            state = get_state()

            if get_alive_middleware_reduce()(state):
                return HttpResponse('I\'m alive!')
            else:
                return HttpResponse('I\'m dead', status=555)
        elif self.status_re.match(request.path):
            return state_detail(request)
