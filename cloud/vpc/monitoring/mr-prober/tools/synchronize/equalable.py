import copy
from typing import TypeVar, Dict, Iterable, Union

from frozendict import frozendict
from pydantic import BaseModel

__all__ = ["EqualableMixin"]

Model = TypeVar("Model", bound=BaseModel)
T = TypeVar("T")


class EqualableMixin:
    def equals(self: Model, other: Model) -> bool:
        return EqualableMixin.normalize(self) == EqualableMixin.normalize(other)

    @staticmethod
    def normalize(obj: Model) -> Dict:
        # First, we remove specified keys from dicts
        # See https://pydantic-docs.helpmanual.io/usage/exporting_models/#advanced-include-and-exclude about the syntax
        # By default we remove only "id" key, but you can override this behavior in subclass via defining Config.comparator_ignores_keys.
        result = obj.dict(exclude=getattr(obj.Config, "comparator_ignores_keys", {"id"}))

        # Second, we convert lists to sets, because order of elements (i.e. ProberConfigs in Prober) isn't important
        return EqualableMixin.convert_lists_to_sets(result)

    @staticmethod
    def equal_sets(first_set: Iterable[Model], second_set: Iterable[Model]):
        first_set_normalized = set()
        for obj in first_set:
            first_set_normalized.add(EqualableMixin.normalize(obj))

        second_set_normalized = set()
        for obj in second_set:
            second_set_normalized.add(EqualableMixin.normalize(obj))

        return first_set_normalized == second_set_normalized

    @staticmethod
    def convert_lists_to_sets(obj: T) -> Union[T, frozenset, frozendict]:
        if isinstance(obj, list):
            return frozenset(EqualableMixin.convert_lists_to_sets(elem) for elem in obj)

        if isinstance(obj, dict):
            result = copy.deepcopy(obj)
            for key, value in list(result.items()):
                result[key] = EqualableMixin.convert_lists_to_sets(value)
            # We use frozendicts because dicts are unhashable and can not be put in set
            return frozendict(result)

        return obj
