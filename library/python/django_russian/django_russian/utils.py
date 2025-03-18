# -*- coding: utf-8 -*-
import os
import io
import six

from django.utils.encoding import smart_text
from library.python import resource


class DeclensionError(ValueError):
    pass


def _read_forms(postfix):
    data = resource.resfs_read(
        'library/python/django_russian/'
        'django_russian/forms_%s.txt' % postfix
    )
    f = io.BytesIO(data)
    if six.PY2:
        lines = [l.strip().decode('utf-8') for l in f if l.strip() and not l.startswith('#')]
    else:
        lines = []
        for l in f:
            l = l.decode()
            if l.strip() and not l.startswith('#'):
                lines.append(l)

    bits = [l.split(u'\t', 1) for l in lines]
    return [(bit[0], bit[1].split(u'\t')) for bit in bits]


NOUN_FORMS = _read_forms('noun')
ADJECTIVE_FORMS = _read_forms('adjective')
VERB_FORMS = _read_forms('verb')


def _to_form(forms, index, value):
    for old_term, form_bits in forms:
        if value.endswith(old_term):
            new_term = form_bits[index]
            value = value[:-len(old_term)] + new_term
            return value
    else:
        if six.PY2:
            raise DeclensionError('Don\'t know how to decline "%s"' % value.encode('utf-8'))
        else:
            raise DeclensionError('Don\'t know how to decline "%s"' % value)

def qnoun(count, value):
    count = int(count)
    value = smart_text(value)
    if count % 100 in [11, 12, 13, 14]:
        index = 0
    elif count % 10 == 1:
        index = 1
    elif count % 10 in [2, 3, 4]:
        index = 2
    else:
        index = 0
    return _to_form(NOUN_FORMS, index, value)


def qadjective(count, value):
    count = int(count)
    value = smart_text(value)
    if count % 100 == 11:
        index = 1
    elif count % 10 == 1:
        index = 0
    else:
        index = 1
    return _to_form(ADJECTIVE_FORMS, index, value)


def qverb(count, value):
    count = int(count)
    value = smart_text(value)
    if count % 100 == 11:
        index = 1
    elif count % 10 == 1:
        index = 0
    else:
        index = 1
    return _to_form(VERB_FORMS, index, value)
