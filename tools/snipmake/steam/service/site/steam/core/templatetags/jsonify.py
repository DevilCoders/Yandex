# -*- coding: utf-8 -*-

from django import template

from core.actions import common

register = template.Library()


@register.filter(name='jsonify')
def jsonify(json_str):
    if not json_str:
        return {}
    json_obj = common.jsonify(json_str, 'JSONIFY_TPL_LOAD_ERROR')
    return json_obj
