import logging

from django.conf import settings

import cloud.mdb.backstage.lib.apps as apps


logger = logging.getLogger('backstage.main.context_processors')


def static(request):
    return {
        'static_address': settings.CONFIG.static.address,
        'static_version': '',
    }


def from_settings(request):
    return {
        'apps': apps,
        'enabled_apps': settings.ENABLED_BACKSTAGE_APPS,
        'installation': settings.INSTALLATION,
        'ya_metrika_counter_enabled': settings.CONFIG.get('ya_metrika_counter_enabled', False),
    }
