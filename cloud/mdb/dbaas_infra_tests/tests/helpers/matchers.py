"""
Additional Hamcrest matchers.
"""

from collections.abc import Iterable, Mapping
from pprint import pformat

from hamcrest.core.matcher import Matcher


class ObjMismatch(Exception):
    """
    Raised if objects do not match
    """


class DictIsSubsetOf(Matcher):
    """
    Matches if item is a subset of template
    """

    def __init__(self, template, full=False):
        self._template = template
        self._errors = []
        self._full = full

    def _obj_is_subset(self, template, inst):
        """
        Recursively validate each member of instance against a template
        """

        def _both(one, another, types):
            """ Compare two items against type(s) """
            if isinstance(one, types) and isinstance(another, types):
                return True

            if not isinstance(one, types) and not isinstance(another, types):
                return False

            raise ObjMismatch('{0} "{1}" is not comparable with {2} "{3}"'.format(
                type(one), one, type(another), another))

        def _list_is_subset(template, inst):
            t_items = list(template)
            counter = len(inst)
            for item in inst:
                for t_item in t_items[:]:
                    try:
                        if self._obj_is_subset(t_item, item):
                            t_items.remove(t_item)
                            counter -= 1
                            break
                    except Exception:
                        continue

            return counter == 0

        def _ensure_equal_len(inst, template):
            if len(inst) > len(template):
                raise ObjMismatch('{0} contains less elements than {1}'.format(template, inst))
            if len(inst) < len(template):
                raise ObjMismatch('{0} contains more elements than {1}'.format(template, inst))

        try:
            if _both(inst, template, Mapping):
                if self._full:
                    _ensure_equal_len(inst, template)

                for key, val in inst.items():
                    self._obj_is_subset(template[key], val)

            elif _both(inst, template, (str, int, bool, type(None))):
                if inst != template:
                    raise ObjMismatch('{0} != {1}'.format(template, inst))

            elif _both(inst, template, float):
                if abs(inst - template) > 0.01:
                    raise ObjMismatch('{0} != {1}'.format(template, inst))

            elif _both(inst, template, Iterable):
                if self._full:
                    _ensure_equal_len(inst, template)

                if not _list_is_subset(template, inst):
                    raise ObjMismatch('{0} is not a subset of {1}'.format(template, inst))

            else:
                raise ObjMismatch('{0} != {1}'.format(template, inst))
        except (KeyError, IndexError, ValueError) as key:
            raise ObjMismatch(key)
        return True

    def matches(self, item, mismatch_description=None):
        """
        Matches if item is a subset of template
        """
        try:
            return self._obj_is_subset(self._template, item)
        except ObjMismatch as err:
            self._errors.append(err)
        return False

    def describe_mismatch(self, item, mismatch_description):
        """
        Describes actual data received
        """
        mismatch_description.append_text(pformat(item))
        for error in self._errors:
            mismatch_description.append_text('\nerror: {0}'.format(error))

    def describe_to(self, description):
        """
        Describes `expected`
        """
        description.append_text('original: {0}'.format(pformat(self._template)))


def is_subset_of(dict_, full=False):
    """
    Return subset matcher instance
    """
    return DictIsSubsetOf(dict_, full)
