# coding: utf-8
import os
from importlib import import_module

from django.core.management.base import BaseCommand, CommandError
from django.conf import settings
from django.core.exceptions import ImproperlyConfigured

from django_tanker import api, utils


class TankerCommand(BaseCommand):
    requires_system_checks = False

    _tanker_client = None

    def add_arguments(self, parser):
        parser.add_argument('--id', '-i', action='store', default=None,
                            help=u'Project ID in Tanker instance')
        parser.add_argument('--keyset', '-k', action='store', default=None,
                            help=u'Default project keyset'),
        parser.add_argument('--language', '-l', action='append', default=None,
                            help=u'Language')
        parser.add_argument('--use-testing', '-t', action='store', default=None,
                            help=u'Use testing Tanker instance')
        parser.add_argument('--token', '-u', action='store', default=None,
                            help=u'Project token')
        parser.add_argument('--key-not-language', action='store_true',
                            help=u'Suppress Tanker\'s usage of keys as translations')
        parser.add_argument('--unapproved', '-d', action='store_true',
                            help=u'Download unapproved translations')
        parser.add_argument('--dry-run', action='store_true',
                            help=u'Makes no changes/modifications')
        parser.add_argument('--branch', action='store',
                            help=u'Branch to use, defaults to master if does not exist')

    @property
    def tanker(self):
        if not self._tanker_client:
            tanker_urls = (
                api.URLS['stable']
                if self.options['use_testing'] in ['no', '0', False]
                else api.URLS['testing']
            )
            self._tanker_client = api.Tanker(
                project_id=self.options['id'],
                base_url=tanker_urls,
                token=self.options['token'],
                dry_run=self.options['dry_run'],
                include_unapproved=self.options['unapproved']
            )
        return self._tanker_client

    def configure_options(self, **options):
        if settings.configured:
            if settings.SETTINGS_MODULE:
                parts = settings.SETTINGS_MODULE.split('.')
                project = import_module(parts[0])

                basepath = os.path.dirname(project.__file__)
            else:
                basepath = None

            tanker_settings = settings
        else:
            if not options['project_id'] or not options['keyset']:
                raise CommandError('Options --project-id and --keyset are required')

            from django_tanker import settings as tanker_settings

            basepath = os.getcwd()

        options['basepath'] = basepath

        for key, value in options.items():
            if value is None:
                setting_name = 'TANKER_%s' % key.upper()

                if hasattr(tanker_settings, setting_name):
                    options[key] = getattr(tanker_settings, setting_name)

        options['app_keysets'] = tanker_settings.TANKER_APP_KEYSETS

        branch = options.get('branch')
        if branch is None:
            # если ничего не указано - работаем с master
            branch = getattr(settings, 'TANKER_REVISION', 'master')

        options['branch'] = branch

        return options


class DownloadUploadBaseCommand(TankerCommand):
    options = None

    def handle(self, *args, **options):
        self.options = self.configure_options(**options)

        files = []

        for app, (lang, path) in utils.find_translation_files(
                self.options['language'],
                self.options['basepath']):
            if app is None:  # проект
                if self.options['keyset'] is None:
                    raise ImproperlyConfigured('TANKER_KEYSET is required'
                                               ' for project translations')

                try:
                    keyset = self._resolve_keyset(self.options['keyset'], path)
                except KeyError:
                    continue
            else:
                try:
                    keyset = self._resolve_keyset(self.options['app_keysets'][app],
                                                  path)
                except KeyError:
                    continue

            files.append((lang, keyset, path))

        if self.options['dry_run']:
            print('Running dry...')

        print(u'Codebase languages: %s' % ', '.join(set(lang for lang, _, __ in files)))

        for lang, keyset, path in files:
            self.handle_language(lang, keyset, path)

    def handle_language(self, lang, keyset, path):
        raise NotImplementedError

    def _resolve_keyset(self, keyset, path):
        basename = os.path.basename(path)

        try:
            keyset = keyset[basename]
        except TypeError:
            if basename != 'django.po':
                raise KeyError('Only `django.po` is handled')

        return keyset
