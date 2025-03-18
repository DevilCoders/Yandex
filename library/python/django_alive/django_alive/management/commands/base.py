# coding: utf-8
from __future__ import unicode_literals, absolute_import
import six
from collections import namedtuple
from django.core.management.base import BaseCommand

from django_alive.loading import get_group_checks, get_groups


Argument = namedtuple('Argument', 'short long action default type help')
def kwargs(self):
    keys = 'action default type help'.split()
    result = {}
    for key in keys:
        value = getattr(self, key, None)
        if value is not None:
            result[key] = value
    return result
Argument.kwargs = property(kwargs)


class AliveBaseCommand(BaseCommand):

    arguments = [
        Argument('-g', '--group', 'append', [], None, 'Logical group'),
        Argument('-c', '--check', 'append', [], None, 'Check name'),
    ]

    def create_option(self, argument):
        from optparse import make_option

        return make_option(
            argument.short, argument.long,
            **argument.kwargs
        )

    @property
    def option_list(self):
        from django import VERSION

        if VERSION < (1, 8):
            return BaseCommand.option_list + tuple(
                self.create_option(argument) for argument in self.arguments
            )
        return None

    def add_argument(self, parser, argument):
        parser.add_argument(
            argument.short, argument.long,
            **argument.kwargs
        )

    def add_arguments(self, parser):
        for argument in self.arguments:
            self.add_argument(parser, argument)

    def handle(self, **options):
        groups = options['group'] or get_groups()
        checks = options['check']

        checks_by_group = {}

        for group in groups:
            checks_by_group[group] = dict(
                (name, check) for name, check in six.iteritems(get_group_checks(group))
                if not checks or name in checks
            )

        return self.handle_checks(checks_by_group, **options)
