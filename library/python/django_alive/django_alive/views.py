# coding: utf-8
from __future__ import unicode_literals, absolute_import

from collections import OrderedDict

from django.shortcuts import render
from django.conf import settings

from .state import get_state


def state_detail(request):
    state = get_state()

    state_by_group_host = OrderedDict()

    for name, groups in sorted(state.iteritems()):
        for group_name, group in sorted(groups.iteritems()):
            if group_name == 'self':
                continue

            state_by_group_host.setdefault(group_name, OrderedDict())

            for host, stamp in sorted(group.iteritems()):
                state_by_group_host[group_name].setdefault(host, OrderedDict())
                state_by_group_host[group_name][host][name] = stamp

    return render(request, 'alive/state_detail.html', {'state': state,
                                                       'state_by_group_host': state_by_group_host,
                                                       'conf': settings.ALIVE_CONF})
