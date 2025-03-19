"""Module provides unified way to validate any schematics type."""

import contextlib
from typing import Any

from schematics.exceptions import ConversionError, CompoundError, ValidationError
from schematics.types import BaseType, ModelType


@contextlib.contextmanager
def _only_validation_error():
    try:
        yield
    except (CompoundError, ConversionError) as e:
        raise ValidationError(str(e))


def validate_untrusted_data(schematics_type: BaseType, untrusted_data, partial: bool=False, strict: bool=True) -> Any:
    """Validate Schematics objects in the unified way.

    For example:
        validate_untrusted_data(ModelType(MyModel), {...})
        validate_untrusted_data(StringType(min_length=10), "LOL")

    Functions converts ConversionError and CompoundError exceptions to ValidationError.
    """
    with _only_validation_error():
        if isinstance(schematics_type, ModelType):
            model_class = schematics_type.model_class
            model = model_class(raw_data=untrusted_data, strict=strict)
            model.validate(partial=partial)
            return model
        elif isinstance(schematics_type, BaseType):
            if partial or not strict:
                raise ValueError("'partial' and 'strict' flags do not make sense for privitive types.")
            return schematics_type.validate(untrusted_data)
        else:
            raise TypeError("Expected BaseType, got an argument of the type {}.".format(schematics_type.__class__.__name__))
