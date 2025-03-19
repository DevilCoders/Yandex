"""Utility functions for resource labeling"""

import re
import schematics.types as schematics_types
import schematics.exceptions as schematics_exceptions


_MAX_COUNT = 64
_LABEL_RE = re.compile(r"^[a-z][-_\./\\@0-9a-z]{,62}$")
_VALUE_RE = re.compile(r"^[-_\./\\@0-9a-z]{,63}$")


class LabelsType(schematics_types.DictType):
    def __init__(self, *args, **kwargs):
        super().__init__(schematics_types.StringType, *args, **kwargs)

    def validate_labels(self, value, context=None):
        for n, (key, value) in enumerate(value.items()):
            if n >= _MAX_COUNT:
                raise schematics_exceptions.ValidationError("Too many labels.")
            if _LABEL_RE.search(key) is None:
                raise schematics_exceptions.ValidationError("Invalid label key {!r}.".format(key))
            if _VALUE_RE.search(value) is None:
                raise schematics_exceptions.ValidationError("Invalid label value {!r}.".format(value))
