# -*- coding: utf-8 -*-

import urllib

from django import template
from core.settings import HIDEREFERER_FORMAT


register = template.Library()


@register.filter(name='hidereferer')
def hidereferer(url):
    return HIDEREFERER_FORMAT % urllib.urlencode((('', url),))[1:]
