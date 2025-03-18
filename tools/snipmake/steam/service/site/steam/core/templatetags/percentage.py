# -*- coding: utf-8 -*-

from django import template


register = template.Library()


@register.filter(name='percentage')
def percentage(number, base):
    if int(base) == 0:
        return ""
    return ''.join(('(', str(100 * int(number) / int(base)), '%)'))
