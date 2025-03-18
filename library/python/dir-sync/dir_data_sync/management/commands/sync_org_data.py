# coding: utf-8

from datetime import timedelta

from django.core.management.base import BaseCommand
from django.db.models import Q
from django.utils import timezone

from ...tasks.sync_data import sync_dir_data_changes, ProcessChangeEventForOrgTask
from ...models import ChangeEvent


class Command(BaseCommand):
    """
    Синхронизировать данные организаций с Директорией.
    Если в качестве параметра передан ID организации, то синхронизировать только данные этой организации,
    иначе синхронизировать данные всех организаций, процедура обновления которых не выполнялась успешно более 24 часов.
    """
    help = 'Sync data of one organization with given ID or all available organizations with Directory'

    def add_arguments(self, parser):
        super(Command, self).add_arguments(parser)
        parser.add_argument('--id', action='store', dest='dir_org_id',
                            help='Organization ID. '
                                 'Specify it if you want to execute a command for a single organization')

    def handle(self, *args, **options):
        dir_org_id = options.get('dir_org_id')

        if dir_org_id:
            ProcessChangeEventForOrgTask()(dir_org_id=dir_org_id)
        else:
            # для поиска организаций устанавливаем величину таймаута после последнего обновления в 24 часа
            timeout = timezone.now() - timedelta(hours=24)
            dir_org_ids_list = ChangeEvent.objects.filter(
                Q(last_pull_at__lt=timeout) | Q(last_pull_at__isnull=True)).values_list('org__dir_id', flat=True)
            if dir_org_ids_list:
                sync_dir_data_changes(dir_org_ids_list)
