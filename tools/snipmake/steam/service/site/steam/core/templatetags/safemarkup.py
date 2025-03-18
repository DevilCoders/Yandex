# -*- coding: utf-8 -*-

import re

from django import template
from django.utils.html import escape
from django.utils.safestring import mark_safe


register = template.Library()


SAFE_TAGS_SINGLE = (
    r'br\s*/?',
)
SAFE_TAGS_PAIR = (
     ('b', '/b'),
     ('strong', '/strong'),
     (r'a(?:\s+href=&quot;#&quot;)?', '/a'),
     (r'a_content(?:\s+href=&quot;.*?&quot;)?', '/a_content'),
)
SAFE_TAGS_SINGLE_RE = re.compile(
    r'&lt;((?:%s)\s*)&gt;' % '|'.join(SAFE_TAGS_SINGLE),
    re.I | re.U)
SAFE_TAGS_PAIR_RE = [
    re.compile(
        r'&lt;((?:%s)\s*)&gt;(.*?)&lt;((?:%s)\s*)&gt;' % (open_tag, close_tag),
        re.I | re.S | re.U)
    for (open_tag, close_tag) in SAFE_TAGS_PAIR]


@register.filter(name='safemarkup')
def safemarkup(content):
    content = escape(content)
    content = SAFE_TAGS_SINGLE_RE.sub(r'<\1>', content)
    for pair_re in SAFE_TAGS_PAIR_RE:
        content = pair_re.sub(r'<\1>\2<\3>', content)
    content = content.replace('&#39;', "'") \
                    .replace('&quot;', '"') \
                    .replace('&amp;', '&') \
                    .replace('a_content', 'a')
    return mark_safe(content)
