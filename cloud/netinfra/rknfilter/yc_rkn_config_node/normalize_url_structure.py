#!/usr/bin/env python3


import re


def adopt_domain_name_to_suricata_format(decomposed_rule):
    if decomposed_rule.blocking_type == "domain-mask":
        domain = adopt_domain_mask_to_suricata_format(decomposed_rule.domain)
    else:
        domain = decomposed_rule.domain
    if contains_cyrillic_symbols(domain):
        domain = get_punycode_representation_string(domain)
    return domain


def adopt_domain_mask_to_suricata_format(domain_mask):
    return domain_mask.replace("*.", "")


def adopt_url_path_to_suricata_format(decomposed_rule):
    path = remove_trailing_garbage_from_url_path(decomposed_rule.path)
    if "\\" in path:
        path = invert_backslashes(path)
    if ";" in path:
        path = cut_path_after_semicolon(path)
    if " " in path:
        path = replace_spaces_in_url_path_with_unicode(path)
    if '"' in path:
        path = replace_double_quote_url_path_with_unicode(path)
    if "'" in path:
        path = replace_single_quote_url_path_with_unicode(path)
    if contains_cyrillic_symbols(path):
        path = get_unicode_representation_string(path)
    return path


def cut_path_after_semicolon(path):
    path.split(";")
    return path[0]


def invert_backslashes(path):
    return path.replace("\\", "/")


def remove_trailing_garbage_from_url_path(path):
    if path[-1] == '"':
        path = path[:-1]
    if path[0] == '"':
        path = path[1:]
    # path = path.replace('"', '')
    if path[-1] == "/":
        path = path[:-1]
    return path


def contains_cyrillic_symbols(text):
    return bool(re.search("[\u0400-\u04FF]", text))


def get_punycode_representation_string(domain):
    return domain.encode("idna").decode("utf-8")


def get_unicode_representation_string(path):
    unicode_string_with_spec_symbols = str(path.encode(encoding="utf-8"))
    unicode_strings = re.search(
        "^[b]['](?P<target>.+)[']$", unicode_string_with_spec_symbols
    )
    return unicode_strings.groupdict()["target"].replace("\\x", "%")


def replace_spaces_in_url_path_with_unicode(path):
    return path.replace(" ", "%20")


def replace_double_quote_url_path_with_unicode(path):
    return path.replace('"', "%22")


def replace_single_quote_url_path_with_unicode(path):
    return path.replace("'", "%27")
