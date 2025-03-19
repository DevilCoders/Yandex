"""
Matchers
"""
from typing import Any

import jsonpath_rw as JSP
from hamcrest import equal_to
from hamcrest.core.base_matcher import Matcher


class JsonPathSyntaxError(Exception):
    """
    Json path syntax error
    """


class MatchJsonPath(Matcher):
    """
    Matcher for json path
    """

    def __init__(self, js_path_str: str, matcher: Any) -> None:
        try:
            self.js_path = JSP.parse(js_path_str)
        except Exception as exc:
            raise JsonPathSyntaxError('Invalid json path: <{0}>: {1}'.format(js_path_str, exc))
        if matcher is not None and not isinstance(matcher, Matcher):
            matcher = equal_to(matcher)
        self.matcher = matcher  # type: Matcher

    def matches(self, item, mismatch_description=None):
        found_results = self.js_path.find(item)
        if not found_results:
            if mismatch_description:
                mismatch_description.append_text('nothing found in ').append_description_of(item)
            return False

        if self.matcher is not None:
            for found_item in found_results:
                if not self.matcher.matches(found_item.value, mismatch_description):
                    return False
        return True

    def describe_mismatch(self, item, mismatch_description):
        self.matches(item, mismatch_description)

    def describe_to(self, description):
        if self.matcher is None:
            description.append_text('data exists at <%s> path' % self.js_path)
        else:
            self.matcher.describe_to(description)
            description.append_text(' at path <%s>' % self.js_path)


def at_path(js_path: str, matcher: Any = None) -> MatchJsonPath:
    """
    Match `matcher` at `js_path`
    """
    return MatchJsonPath(js_path, matcher)
