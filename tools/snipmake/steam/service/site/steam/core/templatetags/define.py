# -*- coding: utf-8 -*-

from django import template
register = template.Library()


@register.assignment_tag
def define(value):
    return value
