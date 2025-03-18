from hamcrest import has_items, assert_that, equal_to, empty

from antiadblock.adblock_rule_sonar.sonar.lib.config import CONFIG


def test_config_load():
    """
    Looking for base sections
    """
    configuration = CONFIG
    sections = ['yt', 'search_regexps']
    assert_that(configuration.keys(), has_items(*sections))


def test_search_regexps_not_empty():
    search_regexps = CONFIG['search_regexps']
    for regexps in search_regexps.values():
        assert_that(regexps, not empty())


def test_search_regexps_no_capturing_groups():
    """
    Capturing groups in search regexps can cause error.
    """
    search_regexps = CONFIG['search_regexps']
    for regexps in search_regexps.values():
        for regexp in regexps:
            for i in [i for i, letter in enumerate(regexp) if letter == '(']:
                assert_that(regexp[i:i + 3], equal_to('(?:'), regexp)
