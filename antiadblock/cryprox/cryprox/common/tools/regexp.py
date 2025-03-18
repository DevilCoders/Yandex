import re2

URLTEMPLATE = '((?:https?:)?//(?:{}))'
URLTEMPLATE_MATCH = '(?:(?:https?:)?//(?:{}))'
EXPAND = [r'[\'"(]{}[,\\]?[\'" )]',
          r'\!\[CDATA\[\s*{}\s*\]\]',
          r'&quot;{}\\?&quot;',
          r'%22{}%22',
          r'%5C%22{}%5C%22',
          r'\S\,\s{}\s',
          r'<ClickThrough>{}</ClickThrough>']
# relative url should NOT begin with https?:// OR // OR data:
RELATIVE_URL_EXPAND = [r'\\?[\'\"\(]({})\\?[\'\" \)]',
                       r'&quot;({})\\?&quot;']


def reg_exps_match_any(reg_exps_list, string):
    r"""
        >>> reg_exps_match_any([re2.compile('adfox_\d+'), re2.compile('banner_\d+')], 'adfox_123 test')
        True
        >>> reg_exps_match_any([re2.compile('adfox_\d+'), re2.compile('banner_\d+')], 'banner_123 test')
        True
        >>> reg_exps_match_any([re2.compile('adfox_\d+'), re2.compile('banner_\d+')], 'test adfox_123')
        False
    """
    for reg_exp in reg_exps_list:
        if reg_exp.match(string):
            return True
    return False


def re_merge(regex):
    r"""
        >>> re_merge(['context(?:_adb)?.js', 'AdFox_\d+'])
        'context(?:_adb)?.js|AdFox_\d+'
    """
    if isinstance(regex, (list, tuple)):
        return '|'.join(regex)
    return regex


def re_completeurl(regexs, strict=False, match_only=False):
    """
        >>> re_completeurl('yandex.ru/pogoda/.*?')
        '((?:https?:)?//(?:yandex.ru/pogoda/.*?))'
        >>> re_completeurl('yandex.ru/pogoda/.*?', match_only=True)
        '(?:(?:https?:)?//(?:yandex.ru/pogoda/.*?))'
        >>> re_completeurl('auto.ru/(static)/main.js')
        '((?:https?:)?//(?:auto.ru/(static)/main.js))'
        >>> re_completeurl(['auto.ru/(static)/main.js', 'yandex.ru/adv/advsdk.js'])
        '((?:https?:)?//(?:auto.ru/(static)/main.js|yandex.ru/adv/advsdk.js))'
        >>> re_completeurl('yandex.ru/pogoda/.*?', True)
        '^((?:https?:)?//(?:yandex.ru/pogoda/.*?))$'
        >>> re_completeurl('auto.ru/(static)/main.js', True)
        '^((?:https?:)?//(?:auto.ru/(static)/main.js))$'
        >>> re_completeurl(['auto.ru/(static)/main.js', 'yandex.ru/adv/advsdk.js'], True)
        '^((?:https?:)?//(?:auto.ru/(static)/main.js|yandex.ru/adv/advsdk.js))$'
    """
    template = URLTEMPLATE_MATCH if match_only else URLTEMPLATE
    if strict:
        return '^' + template.format(re_merge(regexs)) + '$'
    return template.format(re_merge(regexs))


def re_expand(regexs):
    return '|'.join([ex.format(re_completeurl(regexs)) for ex in EXPAND])


def re_expand_relative_urls(regexs):
    return '|'.join([ex.format(re_merge(regexs)) for ex in RELATIVE_URL_EXPAND])


def optimize_replace_dict(replace_dict):
    """
    groups dict keys by value merging keys with pipe ("|") to produce the-sole-big-regexp
    e.g.: {'a':1, 'b':1, 'c':2} => {"a|b":1, "c":2}
    """
    result = dict()
    for value in list(set(replace_dict.values())):
        result[re_merge([k for k, v in replace_dict.iteritems() if v is value])] = value
    return result


def compile_replace_dict(replace_dict):
    result = dict()
    for regex, rpart in optimize_replace_dict(replace_dict).items():
        result[re2.compile(regex)] = rpart
    return result
