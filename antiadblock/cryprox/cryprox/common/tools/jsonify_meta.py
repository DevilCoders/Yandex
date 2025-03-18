# coding=utf-8
import json

import re2


JS_FIXES = [(re2.compile(k), v) for k, v in [
    # no double commas
    (r",(?:\s*,)+", ","),
    # no trailing commas
    (r",\s*([\]}])", r"\1"),
    # escape double quotas inside single quotas for future safe removing of single quotas
    (r"""([:,{[]\s*\\?'.*?)\\?"(.*?)\\?"(.*?\\?'\s*[,\]}])""", r"""\1\\"\2\\"\3"""),
    # no single quotas in keys and values. Escaped ' and " were taken into consideration
    (r"""([{[,:]\s*)\\?'((?:[-:\/\w\s]|\\["'])*)\\?'\s*""", "\\1\"\\2\""),
    # put single keys into quotas. Expect no single quotas. Double regexp for sequential keys case: {a:{b:""}}
    # finds all char sequence that starts from [{[,] and ends with  ': ["[{0-9]|true|false'. Puts it into double quotes
    (r"""([{,]\s*)([a-zA-Z_][-\\/\w]*)(\s*:\s*)(["[{\d]|true|false)""", r"""\1"\2"\3\4"""),
    (r"""([{,]\s*)([a-zA-Z_][-\\/\w]*)(\s*:\s*)(["[{\d]|true|false)""", r"""\1"\2"\3\4"""),
    # escape \'
    # https://st.yandex-team.ru/ANTIADB-1437#5c2745aeeb7243001b207b14 п. 1
    (r"\\'", r"'"),
]]


def jsonify_meta(meta_body, js_obj_extract_regexp, metrics=None, action=None):
    """
    Accepts string, applies js_obj_extract_regexp and jsonify first group (which is expected to be a js-compatible object)

    :param meta_body:
    :param js_obj_extract_regexp:
    :return: parsed meta_body json loaded into python dict
    """

    meta_matcher = js_obj_extract_regexp.match(meta_body)

    if not meta_matcher:
        result = json.loads(meta_body)
        if metrics is not None:
            metrics.increase_counter(action, validate='Valid JSON without callback')
        return result

    new_meta = meta_matcher.group(1)
    try:
        # пытаемся распарсить оригинальный ответ (но без куска с callback)
        # https://st.yandex-team.ru/ANTIADB-1437#5c2745aeeb7243001b207b14 п. 2
        result = json.loads(new_meta)
        if metrics is not None and action is not None:
            metrics.increase_counter(action, validate='Valid JSON with callback')
        return result
    except Exception:
        # пытаемся исправить невалидный json
        for regex, r_part in JS_FIXES:
            new_meta = regex.sub(r_part, new_meta)
        result = json.loads(new_meta)
        if metrics is not None and action is not None:
            metrics.increase_counter(action, validate='Invalid JSON with callback')
        return result
