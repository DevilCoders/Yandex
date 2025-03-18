# -*- coding: utf-8 -*-

from django import template
from django.utils import translation

from core.settings import GEOBASE
from core.hard.loghandlers import SteamLogger


register = template.Library()


@register.filter(name='regionname')
def regionname(region_id):
    if region_id == 0:
        return ''
    cur_language = str(translation.get_language())
    try:
        region_name = GEOBASE.linguistics(int(region_id),
                                          cur_language).nominative
    except Exception as e:
        region_name = ''
        SteamLogger.warning('Could not get region name'\
            ' with id "%(region_id)s" in "%(cur_language)s"',
            region_id=str(region_id), cur_language=cur_language,
            type='GEOBASE_ERROR')
    if not region_name:
        region_name = translation.ugettext('Unknown region: %(region_id)s') %\
            {'region_id': str(region_id)}
    return region_name
