# coding: utf-8

import logging
import six

from django.core.management import BaseCommand

from ...dir_client import DirClient
from ...utils import import_dir_organization


logger = logging.getLogger(__name__)


class Command(BaseCommand):
    """
    Инициировать данные либо одной организации с переданным ID либо всех организаций из Директории с подключенным
    сервисом.
    """
    help = 'Import data of organizations list from Directory'

    def add_arguments(self, parser):
        super(Command, self).add_arguments(parser)
        parser.add_argument('--id', action='store', dest='dir_org_id',
                            help='Organization ID. '
                                 'Specify it if you want to execute a command for a single organization')

    def handle(self, *args, **options):
        dir_org_id = options.get('dir_org_id')

        if dir_org_id:
            organizations = [DirClient().get_organization(dir_org_id)]
        else:
            organizations = DirClient().get_organizations()

        for org in organizations:
            organization = import_dir_organization(org)
            if organization:
                print('Organisation {0} was imported at {1}'.format(
                    six.text_type(organization), organization.created
                ))
            else:
                print('Could not import organization {0}'.format(org['id']))
