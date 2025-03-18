# -*- coding: utf-8 -*-

import urllib

from django import template


register = template.Library()


@register.filter(name='unquote')
def unquote(url):
    try:
        res = urllib.unquote(str(url)).decode('utf8')
    except UnicodeDecodeError:
        res = url
    return res
