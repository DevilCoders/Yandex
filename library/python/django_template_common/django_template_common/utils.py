# -*- coding:utf-8 -*-

import re

from django.utils.safestring import mark_safe
try:
    from django.utils.encoding import smart_text
except ImportError:
    # NOTE smart_text was deprecated in Django3 and removed in Django4
    from django.utils.encoding import smart_str as smart_text

from django.utils.html import conditional_escape
from django.contrib.sites.models import Site

_first_letter = re.compile(r'^(.*?)(\w|\S(?!.*\w))(.*)$', re.UNICODE|re.DOTALL)
def _username(login, html, autoescape = None):
    '''
    Заворачивает «первую букву» и остальную часть имени пользователя в кусок html.
    В переменной html должно быть три placeholder-а %s для подстановки всех небуквенных символов
    в начале, первой буквы и всего остального, например, six.u('%s<b>%s</b>%s')
    http://YandexWiki/Jaru/КраснаяБукваПользователя
    '''
    if not login:
        return mark_safe('')
    match = _first_letter.match(smart_text(login))
    if not match:
        if autoescape:
            login = conditional_escape(login)
        return mark_safe(login)
    groups = match.groups()
    if autoescape:
        groups = tuple(map(conditional_escape, groups))
    return mark_safe(html % groups)

def absolute_url(url):
    if url.startswith('http://') or url.startswith('https://'):
        return url
    return 'http://%s%s' % (Site.objects.get_current().domain, url)
