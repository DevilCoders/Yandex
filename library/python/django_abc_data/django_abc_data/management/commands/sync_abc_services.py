# -*- coding: utf-8 -*-

import logging

from django.core.management.base import BaseCommand

from django_abc_data.lock import lock
from django_abc_data.core import sync_services

log = logging.getLogger(__name__)


class Command(BaseCommand):
    help = u"""Синхронизирует список сервисов с ABC"""

    def add_arguments(self, parser):
        parser.add_argument(
            '--no-update',
            action='store_true',
            default=False,
            help=u'Не обновлять существующие сервисы.',
        )
        parser.add_argument(
            '--no-create',
            action='store_true',
            default=False,
            help=u'Не добавлять новые сервисы.',
        )
        parser.add_argument(
            '--no-delete',
            action='store_true',
            default=False,
            help=u'Не удалять старые сервисы.',
        )
        parser.add_argument(
            '--services',
            default=None,
            help=u'Список external_id сервисов, которые надо синхронизировать.',
        )

    @lock
    def handle(self, *args, **options):
        external_ids = None
        if options['services']:
            external_ids = [int(x) for x in options['services'].split(',')]

        actions = {
            u'delete': not options['no_delete'],
            u'create': not options['no_create'],
            u'update': not options['no_update'],
        }
        sync_services(external_ids=external_ids, **actions)
