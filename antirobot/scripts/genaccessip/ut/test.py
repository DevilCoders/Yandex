import pytest
import yatest.common

import re
import urllib

import antirobot.scripts.genaccessip.genaccessip as genaccessip


@pytest.mark.parametrize(("input", "expect"), [
    ('::1', True),
    ('ff::1', True),
    ('ABcd:0:34:fF', True),
    ('ABcd::0:34:fF', True),
    ('1234:123:0::/64', True),
    ('1234:123:FFFF/64', True),
    ('1::/4', True),
    ('0G:11/4', False),
    ('AB:12:32:56:78:90:01:12/1', True),
    ('AB:12:32:56:78:90:01:12/127', True),
    ('AB:12:32:56:78:90:01:12/128', False),
    ('AB:12:32:56:78:90:01:12:/1', False),
    ('AB:12:32:56:78:90:01:12:11', False),
])
def test_is_valid_ip6_addr(input, expect):
    print "START"
    print dir(genaccessip)
    print "END"
    assert genaccessip.is_valid_ip6_addr(input) == expect


@pytest.mark.parametrize(("input", "expect"), [
    ('1.2.3.4', True),
    ('1.2.3.4/123', True),
    ('1.2.3.4/ 123', False),
    ('000.00.0.255', True),
    ('a.b.c.d', False),
    ('1.2.3', False),
    ('1.2.3..4', False),
])
def test_is_ip4(input, expect):
    assert genaccessip.is_ip4(input) == expect


@pytest.mark.parametrize(("input", "expect"), [
    ('abc.def', True),
    ('abc.def.ghi', True),
    ('a1.b2.c', True),
    ('a1.b2.3', False),
    ('abcd', False),
])
def test_is_host(input, expect):
    assert genaccessip.is_host(input) == expect


@pytest.mark.parametrize(("input", "expect"), [
    ('_macro_', True),
    ('_macro', False),
    ('macro_', False),
    ('macro', False),
    ('macro.com', False),
    ('_1_', False),
    ('_a1_', True),
])
def test_is_macro(input, expect):
    assert genaccessip.is_macro(input) == expect


def test_translate_right_output():
    assert genaccessip.translate("1.2.3.4") == "1.2.3.4\n"


def test_translate_unchanged():
    lines = ['1.2.3.4', '::1', '1234:5678:ff:FF::/64', '']
    joined_lines = "\n".join(lines)
    assert genaccessip.translate(joined_lines) == joined_lines


@pytest.mark.parametrize(("line"), [
    'sandbox.yandex-team.ru',
    '_TANKNETS_',
    'https://racktables.yandex.net/export/networklist.php?report=usernets',
    'https://developers.google.com/search/apis/ipranges/googlebot.json?hl=ru',
    'https://www.gstatic.com/ipranges/goog.json'
])
def test_translated(line):
    translated = genaccessip.translate(line)

    translated_lines = [x.strip() for x in translated.split('\n')]
    non_comment_line_regexp = re.compile("^[^#]+", re.MULTILINE)

    assert translated_lines != [line]
    assert non_comment_line_regexp.search(translated) is not None


@pytest.mark.parametrize(("file_name"), [
    'antirobot/scripts/support/privileged_ips',
    'antirobot/scripts/support/special_ips.txt',
    'antirobot/scripts/support/trbosrvnets.txt',
    'antirobot/scripts/support/whitelist_ips.txt',
    'antirobot/scripts/support/yandex_ips.txt',
])
def test_all_macros_exist(file_name):
    macros_url = 'http://ro.admin.yandex-team.ru/data/macros-inc.m4'
    non_macro_character_regexp = '[^A-Za-z\\d_]'

    macro_definitions = urllib.urlopen(macros_url).read()

    path_to_file = yatest.common.source_path(file_name)

    unknown_macros = []

    for line in open(path_to_file):
        line = line.strip()
        if line.startswith('_'):
            macro = line.split('#', 1)[0].strip()
            regexp = re.compile(non_macro_character_regexp + macro + non_macro_character_regexp)
            if not regexp.search(macro_definitions):
                unknown_macros.append(macro)

    assert len(unknown_macros) == 0
