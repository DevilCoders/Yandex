from datetime import datetime, date, time, timedelta

from hamcrest.core.base_matcher import BaseMatcher


class IsDateCloseTo(BaseMatcher):
    DATE_TYPES = (datetime, date, time)

    def __init__(self, value, delta):
        if not isinstance(value, self.DATE_TYPES):
            raise TypeError('IsDateCloseTo value must be on of {}'.format(self.DATE_TYPES))
        if not isinstance(delta, timedelta):
            raise TypeError('IsDateCloseTo delta must be of `datetime.timedelta` type')

        self.value = value
        self.delta = self.time_abs(delta)

    @staticmethod
    def time_abs(delta):
        if delta >= timedelta():
            return delta
        return timedelta() - delta

    def _matches(self, item):
        if not isinstance(item, self.DATE_TYPES):
            return False
        return self.time_abs(item - self.value) <= self.delta

    def describe_mismatch(self, item, mismatch_description):
        if not isinstance(item, self.DATE_TYPES):
            super(IsDateCloseTo, self).describe_mismatch(item, mismatch_description)
        else:
            actual_delta = self.time_abs(item - self.value)
            mismatch_description.append_description_of(item)            \
                                .append_text(' differed by ')           \
                                .append_description_of(actual_delta)

    def describe_to(self, description):
        description.append_text('a date/time/datetime value within ')  \
                   .append_description_of(self.delta)       \
                   .append_text(' of ')                     \
                   .append_description_of(self.value)


def date_close_to(value, delta):
    """Matches if object is a date/time point close to a given value, within a given
    delta.

    :param value: The value to compare against as the expected value.
    :param delta: The maximum delta between the values for which the numbers
        are considered close.

    This matcher compares the evaluated object against ``value`` to see if the
    difference is within a positive ``delta``.

    Example::
        date_close_to(datetime.now(), timedelta(seconds=1))
    """
    return IsDateCloseTo(value, delta)
