# -*- coding:utf-8 -*-
import os
import re
from datetime import datetime
import six
from six.moves.urllib_parse import urljoin

from django import template
from django.conf import settings
from django.utils.html import conditional_escape
from django.utils.safestring import mark_safe
from django.utils.http import urlencode
from django.utils.html import escape

if six.PY2:
    from django.utils.encoding import smart_unicode
else:
    def smart_unicode(string):
        return string

from django_template_common import utils

register = template.Library()

class PaginatorNode(template.Node):
    def __init__(self, preserve_keys):
        self.preserve_keys = preserve_keys

    def render(self, context):
        paginator = context['paginator']
        page = context['page_obj']
        context['previous_list'] = [p + 1 for p in range(page.number - 1)]
        context['next_list'] = [p + 1 for p in range(page.number - 1 + 1, paginator.num_pages)]
        t = template.loader.get_template('paginator.html')
        result = t.render(context)

        preserve_values = dict([(k, context[k]) for k in self.preserve_keys if k in context])
        # Hack: converting timeless datetimes to a nice format
        for k in preserve_values:
            if isinstance(preserve_values[k], datetime):
                dt = preserve_values[k]
                if dt.hour == dt.minute == dt.second == 0:
                    preserve_values[k] = dt.strftime('%Y-%m-%d')
        if preserve_values:
            param_str = six.u('&amp;') + escape(urlencode(preserve_values, True))
            result = re.sub(six.u(r'(\?page=\d*)'), six.u(r'\1') + param_str, result)

        return result

@register.tag
def paginator(parser, token):
    '''
    Выводит навигатор по страницам. Данные берет из контекста в том же
    виде, как их туда передает generic view "object_list".
    '''
    bits = token.contents.split()
    if len(bits) > 1:
        preserve_keys = bits[1].split(',')
    else:
        preserve_keys = []
    return PaginatorNode(preserve_keys)

# Теги js и css, а также функция _static устарели. Вместо них нужно
# использовать тег media

def _static_url(kind, name):
    import time
    try:
        root = getattr(settings, kind.upper() + '_ROOT')
    except AttributeError:
        root = os.path.join(settings.MEDIA_ROOT, kind)
    filename = os.path.join(root, re.sub(r'\?.*', '', name))
    if not os.path.exists(filename):
      return ''
    modified_time = datetime(*time.gmtime(os.path.getmtime(filename))[:6])
    return '%s%s/%s?%s' % (settings.MEDIA_URL, kind, name, modified_time.strftime('%Y%m%d%H%M%S'))

@register.simple_tag
def js(name):
    '''
    Составляет ссылку на статический js-файл с добавлением времени обновления
    файла параметром после '?'.
    '''
    return _static_url('js', name)

@register.simple_tag
def css(name):
    '''
    Составляет ссылку на статический css-файл с добавлением времени обновления
    файла параметром после '?'.
    '''
    return _static_url('css', name)

@register.simple_tag
def media(filename, flags=''):
    '''
    Состаляет ссылку на статический файл, присоединяя его к MEDIA_URL, если
    надо:

        {% media "css/styles.css" %} -> /media/css/styles.css
        {% media "/robots.txtf" %} -> /robots.txt
        {% media "http://images.yandex.ru/blank.gif" %} -> http://images.yandex.ru/blank.gif

    Понимает как литеральные строки, так и переменные.

    Также принимает три флага:

    -   absolute: строит абсолютный URL со схемой и доменом (использует
        contrib.sites)
    -   timestamp: добавляет timestamp в параметры URL'а, если находит этот
        статический файл на диске в MEDIA_ROOT. Для .css и .js файлов это
        поведение по умолчанию.
    -   no-timestamp: соответственно, не выводит timestamp для .css и .js
        файлов

    Несколько флагов можно указывать через запятую:

        {% media "img/success.jpg" "absoulte, timestamp" %}
    '''
    flags = set(f.strip() for f in flags.split(','))
    url = urljoin(settings.MEDIA_URL, filename)
    if 'absolute' in flags:
        url = utils.absolute_url(url)
    if (filename.endswith('.css') or filename.endswith('.js')) and 'no-timestamp' not in flags or \
       'timestamp' in flags:
        fullname = os.path.join(settings.MEDIA_ROOT, filename)
        if os.path.exists(fullname):
            url += '?%d' % os.path.getmtime(fullname)
    return url

@register.simple_tag
def userpic_url(username, size='middle'):
    '''
    Формирует ссылку на текущую аватарку полльзователя. Принимает имя
    пользователя и опционально -- размер в виде "small", "middle" и "normal".
    Если размер не задан, используется "middle".
    '''
    if size not in ('small', 'middle', 'normal'):
        raise Exception('Incorrect userpic size: %s' % size)
    url = 'http://upics.yandex.net/get/%s/%s/'
    return mark_safe(url % (conditional_escape(username), size))

@register.filter
def username(login, autoescape = None):
    '''
    Заворачивает «первую букву» имени пользователя в <b>.
    http://YandexWiki/Jaru/КраснаяБукваПользователя
    '''
    return utils._username(login, six.u('%s<b>%s</b>%s'), autoescape)
username.needs_autoescape = True

@register.filter
def username_email(value, autoescape=None):
    '''
    Вариант username но с использованием презентационного HTML'а.
    Используется в письмах.
    '''
    return utils._username(value, six.u('<font color="#000000">%s<font color="#ff0000">%s</font>%s</font>'), autoescape)
username_email.needs_autoescape = True

@register.filter
def truncateword(value, arg):
    '''
    Обрезает длинные слова по максимальному количеству символов, переданному
    в аргументе. Обрезанное слово добивает символом "…".
    '''
    arg = int(arg)
    value = smart_unicode(value)
    if len(value) <= arg:
        return value
    return value[:arg] + six.u('…')
truncateword.is_safe = True

@register.filter
def break_long_words(value, max_word_size=30):
    """
    Разбивает неестественно длинные слова вставлением мягких переносов (<wbr />).
    Длина "длинных" слов передается аргментом. По умолчанию -- 30.
    """
    import re
    r = re.compile("[\S]{%s,}" % max_word_size, re.UNICODE)

    def replace_func(value):
        return '<wbr />'.join([l for l in value.group(0)])

    return mark_safe(re.sub(r, replace_func, value))
break_long_words.is_safe = True
