#!/usr/bin/env python

import sys
import tinycss2
from tinycss2.ast import ParseError
import webencodings
import link_types

class SilentParseError(ParseError):
    type = 'error'

    def __init__(self, error):
       ParseError.__init__(self, error.source_line, error.source_column, error.kind, error.message)

    def _serialize_to(self, write):
        try:
            ParseError._serialize_to(self, write)
        except TypeError:
            pass

def iterate_rules(rules, url_mapper):
    if not rules:
        return rules
    result = []
    for subrule in rules:
        rewrite(subrule, url_mapper)
        if subrule.type != 'error':
            result.append(subrule)
        else:
            result.append(SilentParseError(subrule))
    return result

def patch_url(rule, url_mapper, link_type):
    url = rule.value
    if url and not url.startswith(u'data:'):
        rule.value = url_mapper(url, link_type)

def find_url_or_string(rules):
    for subrule in rules:
        if subrule.type == 'string' or subrule.type == 'url':
            return subrule
    return None

def rewrite(rule, url_mapper):
    if rule.type == 'at-rule':
        if rule.lower_at_keyword == 'import':
            url_rule = find_url_or_string(rule.prelude)
            if url_rule:
                patch_url(url_rule, url_mapper, link_types.CSS)
        else:
            rule.prelude = iterate_rules(rule.prelude, url_mapper)
            rule.content = iterate_rules(rule.content, url_mapper)
    elif rule.type == 'url':
        patch_url(rule, url_mapper, link_types.IMAGE)
    elif rule.type == 'error':
        print >>sys.stderr, 'Parse error at %d:%d (%s): %s' % (rule.source_line, rule.source_column, rule.kind, rule.message)
    elif rule.type == 'qualified-rule':
        rule.prelude = iterate_rules(rule.prelude, url_mapper)
        rule.content = iterate_rules(rule.content, url_mapper)
    elif rule.type == 'declaration':
        rule.value = iterate_rules(rule.value, url_mapper)
    elif rule.type in ('() block', u'[] block', u'{} block'):
        rule.content = iterate_rules(rule.content, url_mapper)
    elif rule.type == 'function':
        rule.arguments = iterate_rules(rule.arguments, url_mapper)

def rewrite_css(css, url_mapper, encoding_name, is_inline):
    if not is_inline:
        rules, encoding = tinycss2.parse_stylesheet_bytes(css, encoding_name)
    else:
        encoding = webencodings.lookup(encoding_name)
        content, encoding = webencodings.decode(css, encoding, errors='replace')
        rules = tinycss2.parse_declaration_list(content)

    def url_mapper_wrapper(url, content_hint):
        url_bytes = webencodings.encode(url, encoding)
        result = url_mapper(url_bytes, content_hint)
        return webencodings.decode(result, encoding)[0]

    rules = iterate_rules(rules, url_mapper_wrapper)

    result_css = tinycss2.serialize(rules)
    result = webencodings.encode(result_css, encoding)
    return result


