# -*- coding: utf-8 -*-
import re
import textwrap


def drop_spaces(text):
    return textwrap.dedent(text).lstrip('\n')


# deprecated, use drop_spaces directrly
def prepare_text(text):
    return drop_spaces(text)


def uppercase(s):
    return str(s).upper()


def camelcase(s):
    s = re.sub(r"^[\-_\.]", "", str(s))
    if not s:
        return s
    return uppercase(s[0]) + re.sub(r"[\-_\.\s]([a-z])", lambda matched: uppercase(matched.group(1)), s[1:])


def underscore(s):
    return re.sub("([A-Z])", "_\\1", re.sub(r"\s+", "", re.sub(r"\.", "_", s))).lower().lstrip("_").lstrip(" ")
