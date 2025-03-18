# coding: utf-8

from __future__ import unicode_literals

from django.http import HttpResponse
from django_alive.middleware import AliveMiddleware
from django_alive.views import state_detail


def check_http_and_db_master_availability(state):
    return (
        all(state['www']['self']) and
        all(
            check for check in state['db-master']['self']
            if check.stamp.group == 'front'
        )
    )


class DummyAliveMiddleware(AliveMiddleware):
    def process_request(self, request):
        if self.ping_re.match(request.path):
            return HttpResponse('I\'m alive!')
        elif self.status_re.match(request.path):
            return state_detail(request)
