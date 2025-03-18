# -*- coding: utf-8 -*-

from django import template
register = template.Library()

from math import ceil
from core.actions.paginator import get_pages


@register.assignment_tag(takes_context=True)
def paginator(context, page_url, pages_info):
    pages_count = int(ceil(pages_info['obj_count'] /
                           float(pages_info['elements_on_page'])))
    pages = get_pages(pages_info['cur_page'], pages_count)
    tpl = template.loader.get_template('core/paginator.html')

    qs = pages_info.get('query_string', '')
    if qs:
        qs = '?' + qs

    return tpl.render(template.Context(
        {'page_url': page_url,
         'pages': pages,
         'qs': qs}))
