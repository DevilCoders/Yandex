# -*- coding: utf-8 -*-

from django import template


register = template.Library()


@register.filter(name='crit_name_cleaner')
def crit_name_cleaner(crit_name):
    return crit_name.replace('_', '-')
