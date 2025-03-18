import json

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.helpers.wrap_matcher import wrap_matcher
from hamcrest.core.matcher import Matcher  # noqa


class IsJsonSerialized(BaseMatcher):
    def __init__(self, matcher):
        """
        :type matcher: Matcher
        """
        self.matcher = matcher

    def _matches(self, item):
        try:
            obj = json.loads(item)
        except:
            return False
        return self.matcher.matches(obj)

    def describe_mismatch(self, item, mismatch_description):
        return self.matcher.describe_mismatch(item, mismatch_description)

    def describe_to(self, description):
        description.append_description_of(self.matcher)


def is_json_serialized(x):
    return IsJsonSerialized(wrap_matcher(x))
