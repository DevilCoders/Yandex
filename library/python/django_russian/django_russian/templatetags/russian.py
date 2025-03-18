# -*- coding: utf-8 -*-
import warnings
import datetime as dt
import six
from django.conf import settings
from django.utils.encoding import smart_text, force_text, smart_str
import django.template as template
from django_russian import utils

class NotAvailableWarning(UserWarning): pass

try:
    from inflect import inflect as yandex_inflect
except ImportError:
    yandex_inflect = None

register = template.Library()

def _num(n, base, post_1, post_2_4, post_5_0):
    dzat = (((n//10) % 10) == 1)

    if dzat or n % 10 in [5, 6, 7, 8, 9, 0]:
        return '%s%s' % (base, post_5_0)
    elif n % 10 in [2, 3, 4]:
        return '%s%s' % (base, post_2_4)
    elif n % 10 == 1:
        return '%s%s' % (base, post_1)

def _n_num(n, base, post_1, post_2_4, post_5_0):
    return '%i %s' % (n, _num(n, base, post_1, post_2_4, post_5_0))

MONTHS = {
    1: u'января',
    2: u'февраля',
    3: u'марта',
    4: u'апреля',
    5: u'мая',
    6: u'июня',
    7: u'июля',
    8: u'августа',
    9: u'сентября',
    10: u'октября',
    11: u'ноября',
    12: u'декабря',
}


@register.filter
def human_date(date, mode=""):
    '''
    Дата в привычном русскоязычном виде типа "15 августа". Если
    год отличен от текущего, он добавляется.
    mode - режимы вывода в строку через пробел
        no_year - не вводить год в любом случае
        short_month - месяц тремя буквами
    '''
    if date is None:
        return u''
    try:
        now = dt.datetime.now()
        if now.year == date.year or 'no_year' in mode:
            format = u'%(day)s %(month)s'
        else:
            format = u'%(day)s %(month)s %(year)s'

        month = MONTHS[date.month]
        if 'short_month' in mode:
            month = month[:3]

        return format % dict(day = date.day,
                             month = month,
                             year = date.year)
    except AttributeError:
        if not settings.DEBUG:
            return smart_text(date)
        raise

@register.filter
def human_datetime(date):
    '''
    Время в разговорном формате ("только что", "вчера", "минуту назад").
    Если подходящего относительного вида не находится, то время выводится в
    виде human_date ("15 августа").
    '''
    if date is None:
        return u''
    try:
        now = dt.datetime.now()
        diff = now - date

        def to_date(d): return d.replace(hour=0, minute=0, second=0, microsecond=0)
        d_diff = to_date(now) - to_date(date)

        if diff < dt.timedelta(minutes=1):
            return u'только что'
        elif diff < dt.timedelta(hours=1):
            return u'%s назад' % _n_num(diff.seconds//60, u'минут', u'у', u'ы', u'')
        elif diff < dt.timedelta(days=1) and d_diff.days == 0:
            return u'%s назад' % _n_num(diff.seconds//3600, u'час', u'', u'а', u'ов')
        elif d_diff.days == 1:
            return u'вчера'
        elif d_diff.days == 2:
            return u'позавчера'
        else:
            return human_date(date)
    except AttributeError:
        if not settings.DEBUG:
            return smart_text(date)
        raise


@register.filter
def human_duration(delta, mode=None):
    '''
    Перевод длительности из timedelta в человеческий формат
    Вход: длительность в timedelta
    Выход: длительность в виде '1 день 6 часов' или '4 часа 23 минуты'

    mode:
    round_hours - округлять ли длительность до целых часов
    short - выводить минуты как "мин"
    '''
    if not delta:
        return ''

    minutes = delta.seconds / 60
    hours = 0
    days = delta.days

    if minutes > 59:
        hours = minutes / 60
        minutes = minutes % 60

    # Если есть дни, минуты округляем до часов
    if (days and mode == 'round') or mode == 'force_round':
        hours += int(round(float(minutes) / 60))
        minutes = 0

        # Если получилось 24 часа, то прибавляем день и убираем часы
        if hours == 24:
            days += 1
            hours = 0

    blocks = []
    if days:
        blocks.append("%d %s" % (days, utils.qnoun(days, u"день")))

    if hours:
        blocks.append("%d %s" % (hours, utils.qnoun(hours, u"час")))

    if minutes and len(blocks) < 2:
        blocks.append("%d %s" % (minutes, mode == 'short' and u"мин" or utils.qnoun(minutes, u"минута")))

    if len(blocks):
        return ' '.join(blocks)
    else:
        return '-'

@register.simple_tag
def human_date_interval(from_date, to_date):
    """
    Человеческое обозначение интервала из начальной и конечной дат
    """
    if from_date.year != dt.datetime.now().year:
        year_str = u' %s года' % from_date.year
    else:
        year_str = u''

    if from_date == to_date:
        return u'%s %s' % (from_date.day, MONTHS[from_date.month]) + year_str
    elif from_date.year == to_date.year and from_date.month == to_date.month:
        return u'%s — %s %s' % (from_date.day, to_date.day, MONTHS[to_date.month]) + year_str
    elif from_date.year == to_date.year:
        return u'%s %s — %s %s' % (from_date.day, MONTHS[from_date.month], to_date.day, MONTHS[to_date.month]) + year_str
    else:
        return u'%d.%02d.%d — %d.%02d.%d' % (from_date.day, from_date.month, from_date.year, to_date.day, to_date.month, to_date.year)

@register.filter
def human_file_size(sz):
    '''
    Форматирование числа байт в коротком удобочитаемом виде в килобайтах,
    мегабайтах и гигабайтах.
    '''
    sz = int(sz)
    sizes = [(1., u'б'),
             (1024., u'Кб'),
             (1024.*1024., u'Мб'),
             (1024.*1024.*1024., u'Гб')]

    for k, n in sizes:
        if sz/k < 1024:
            return u'%.1f %s' % (sz/k, n)

    return u'%.1f %s' % (sz/k, n)

@register.filter
def smart_join(ss):
    '''
    Перечисление элементов списка через запятую, кроме последнего элемента,
    перед которым вставляется "и". Получается "а, б и в".
    '''
    ss = [smart_text(i) for i in ss]

    if len(ss) >= 2:
        res = u'%s и %s' % (', '.join(ss[0:-1]), ss[-1])
    elif len(ss) == 1:
        res = ss[0]
    else:
        res = u''

    return res

@register.simple_tag
def quantity(count, value):
    '''
    Склоняет слова относительно числительного ("1 альбом", "15 альбомов").
    Принимает два параметра: число и слово в именительном падеже.
    '''
    try:
        value = utils.qnoun(count, value)
        return u'<span class="number">%s</span> %s' % (count, value)
    except utils.DeclensionError:
        if six.PY2:
            raise template.TemplateSyntaxError('Don\'t know how to decline "%s"' % value.encode('utf-8'))
        else:
            raise template.TemplateSyntaxError('Don\'t know how to decline "%s"' % value)

class QuantityNode(template.Node):
    def __init__(self, func, value):
        self.func = func
        self.value = value

    def render(self, context):
        value = template.resolve_variable(self.value, context)
        count = context['count']
        return self.func(count, value)

def _qtag(func, parser, token):
    try:
        tag, value = token.split_contents()
    except ValueError:
        raise template.TemplateSyntaxError('%s принимает ровно один аргумент' % token.contents.split()[0])
    return QuantityNode(func, value)

@register.tag
def qnoun(parser, token):
    '''
    Склоняет существительные относительно числительного ("1 альбом", "15 альбомов").
    Принимает два параметра: число и существительное в именительном падеже.
    '''
    return _qtag(utils.qnoun, parser, token)

@register.tag
def qadj(parser, token):
    '''
    Склоняет прилагательное относительно числительного ("1 забаненный", "15 забаненных").
    Принимает два параметра: число и прилагательное в именительном падеже.
    '''
    return _qtag(utils.qadjective, parser, token)

@register.tag
def qverb(parser, token):
    '''
    Спрягает глаголы относительно числительного ("1 [человек] идет", "15 [человек] идут").
    Принимает два параметра: число и глагол в единственном числе.
    '''
    return _qtag(utils.qverb, parser, token)


_inflection_forms = {
    u'кто': 1,
    u'что': 1,
    u'кого': 2,
    u'чего': 2,
    u'кому': 3,
    u'чему': 3,
    u'кого-что': 4,
    u'кем': 5,
    u'чем': 5,
    u'о ком': 6,
    u'о чем': 6,
}

if yandex_inflect is None:
    def inflect(form, word):
        warnings.warn('Package yandex-python-inflect is not installed, inflection filters aren\'t available.', NotAvailableWarning, 2)
        return word
else:
    def inflect(form, word):
        inflected = force_text(yandex_inflect(smart_str(word), _inflection_forms[form]))
        return inflected or word

for form in _inflection_forms:
    register.filter(
        form.replace(' ', '_').replace('-', '_'),
        (lambda form: lambda value: inflect(form, value))(form)
    )

