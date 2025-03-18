# coding: utf-8

from __future__ import unicode_literals, absolute_import
import six
from django.utils.encoding import smart_str

from .loading import get_groups, get_group_checks
from .utils import current_host


class CheckState(object):
    def __init__(self, check, stamp):
        self.check = check
        self.stamp = stamp

    def __bool__(self):
        return self.check.get_stamp_status(self.stamp)

    def __unicode__(self):
        return 'CheckState<%s stamp{%s, %s}>' % (
            'OK' if bool(self) else 'FAIL',
            self.stamp.timestamp,
            self.stamp.data
        )

    def __repr__(self):
        return smart_str(self.__unicode__())


def get_state(groups=None, checks=None):
    state = {}

    for group in get_groups():
        if groups and group not in groups:
            continue

        for name, check in six.iteritems(get_group_checks(group)):
            if checks and name not in checks:
                continue

            state.setdefault(name, {})
            state[name].setdefault('self', [])

            state[name][group] = check.get_state(group=group)
            if current_host in state[name][group]:
                state[name]['self'].append(state[name][group][current_host])

    return state


def default_reduce(state):
    return all(all(check['self']) for check in six.itervalues(state))
