# coding: utf-8
from __future__ import unicode_literals, absolute_import
import six
from multiprocessing.dummy import Pool, cpu_count

from .base import AliveBaseCommand, Argument


class Command(AliveBaseCommand):
    help = 'Do checks'

    arguments = AliveBaseCommand.arguments + [
        Argument('-t', '--threads', 'store', cpu_count(), int, 'Threads count'),
        Argument('-n', '--nothreading', 'store_true', False, None, 'Do not use threads'),
    ]

    def handle_checks(self, checks_by_group, **options):
        pool = Pool(processes=1 if options['nothreading'] else options['threads'])

        def _apply(func, *args, **kwargs):
            if options['nothreading']:
                func(*args, **kwargs)
            else:
                pool.apply_async(func, args, kwargs)

        for group, checkers in six.iteritems(checks_by_group):
            for checker in six.itervalues(checkers):
                _apply(checker.do_check, group)

        pool.close()
        pool.join()
