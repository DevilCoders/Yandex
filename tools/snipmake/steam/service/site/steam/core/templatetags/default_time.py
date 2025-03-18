# -*- coding: utf-8 -*-

from django import template


register = template.Library()


@register.filter(name='default_time')
def default_time(date):
    return date.strftime('00:00:00 %d.%m.%Y')
