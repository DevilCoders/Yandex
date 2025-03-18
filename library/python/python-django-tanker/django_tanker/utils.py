# coding: utf-8
import os
import glob
import re
from django.apps import apps
from django.conf import settings


def transform_path(path):
    app_result = re.match(r'{}(?P<app>.+)/locale'.format(settings.BASE_DIR), path)
    if app_result:
        return app_result.group('app').strip('/').replace('/', '.')


def find_translation_files(languages, basepath=None):
    """Ищет файлы перевода в проекте, приложениях и в специально указанных
    директориях проекта"""
    def _find(path):
        for lang in languages:
            globpath = os.path.abspath(os.path.join(path, lang, 'LC_MESSAGES',
                                                    '*.po'))
            for fullpath in glob.glob(globpath):
                yield lang, fullpath

    for localepath in settings.LOCALE_PATHS:
        transformed_path = transform_path(localepath)
        if transformed_path and any(app.startswith(transformed_path)
                                    for app in settings.INSTALLED_APPS):
            # Этот путь будет корректно обработан позднее на
            # этапе прохода по settings.INSTALLED_APPS
            continue
        if os.path.isdir(localepath):
            for pair in _find(localepath):
                yield None, pair

    app_configs = apps.get_app_configs()

    for app_config in app_configs:
        app_locale_path = os.path.join(app_config.path, 'locale')

        if os.path.isdir(app_locale_path):
            for pair in _find(app_locale_path):
                yield app_config.name, pair

    if basepath and os.path.isdir(basepath):
        for pair in _find(os.path.join(basepath, 'locale')):
            yield None, pair
