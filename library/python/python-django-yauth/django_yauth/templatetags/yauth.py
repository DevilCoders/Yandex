#!/usr/bin/env python
# -*- coding: utf-8 -*-

from django import template

register = template.Library()


@register.inclusion_tag('yauth.html', takes_context=True)
def yauth(context):
    return context


@register.inclusion_tag('login_form.html', takes_context=True)
def login_form(context):
    return context
