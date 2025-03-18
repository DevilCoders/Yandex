# coding: utf-8
from __future__ import unicode_literals, absolute_import
import six

from django.core.management.base import CommandError

from django_alive.state import get_state
from django_alive.utils import import_object

from .base import AliveBaseCommand, Argument


class Command(AliveBaseCommand):
    help = 'Get current checks state'

    arguments = AliveBaseCommand.arguments + [
        Argument('-r', '--reduce', 'store', 'django_alive.state.default_reduce', None,
                 'Path to reduce function'),
        Argument('-o', '--monrun', 'store_true', False, None,
                 'Reduce state and output in monrun format'),
    ]

    def handle(self, **options):
        state = get_state(options['group'], options['check'])
        reducer = import_object(options['reduce'])

        if options['monrun']:
            self.stdout.write('0;OK' if reducer(state) else '2;FAIL')
        else:
            for name, groups in six.iteritems(state):
                groups = dict(groups)
                groups.pop('self')

                if not groups:
                    continue

                for group_name, group in six.iteritems(groups):
                    for host, result in six.iteritems(group):
                        self.stdout.write('%s at %s [%s] -> %s stamp{%s, %s}'
                                          % (name, host, group_name,
                                             'OK' if result else 'FAIL',
                                             result.stamp.timestamp,
                                             result.stamp.data))

                self.stdout.write('')
