"""Module implements well-typed versions of standard schematics types

It is strongly recommended to use these types instead of schematics.types.
In this way you will prevent intrusive PyCharm-warnings and PyCharm inspections will be useful.
"""
from datetime import date, datetime
from decimal import Decimal
from typing import Dict, Iterable, List, Type, TypeVar, Union

from schematics.undefined import Undefined as _Undefined

from yc_common import models as _models


T = TypeVar("T")
K = TypeVar("K")
V = TypeVar("V")


# FIXME: default can be callable
# FIXME: use these types from yc_compute.models, do not use semantics directly
# FIXME: add metadata


# noinspection PyTypeChecker,PyPep8Naming
def BooleanType(default: bool=_Undefined, required: bool=False, **kwargs) -> bool:
    return _models.BooleanType(default=default, required=required, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def DateTimeType(default: datetime=_Undefined, required: bool=False, serialized_format: str=None, **kwargs) -> datetime:
    return _models.DateTimeType(default=default, required=required, serialized_format=serialized_format, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def DateType(default: date=_Undefined, formats: Union[str, Iterable[str]]=None, required: bool=False, **kwargs) -> date:
    return _models.DateType(default=default, formats=formats, required=required, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def DecimalType(default: Decimal=_Undefined, required: bool=False, **kwargs) -> Decimal:
    return _models.DecimalType(default=default, required=required, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def DictType(field: V, key: K=None, required: bool=False, **kwargs) -> Dict[K, V]:
    return _models.DictType(field, key=key, required=required, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def FloatType(default: float=_Undefined, required: bool=False, **kwargs) -> float:
    return _models.FloatType(default=default, required=required, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def IntType(default: int=_Undefined, min_value: int=None, max_value: int=None, required: bool=False, **kwargs) -> int:
    return _models.IntType(default=default, min_value=min_value, max_value=max_value, required=required, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def JsonDictType(field: V, key: K=None, required: bool=False, **kwargs) -> Dict[K, V]:
    return _models.JsonDictType(field, key=key, required=required, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def JsonListType(field: T, default: List[T]=_Undefined, min_size: int=None, max_size: int=None, required: bool=False, **kwargs) -> List[T]:
    return _models.JsonListType(field, default=default, min_size=min_size, max_size=max_size, required=required, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def JsonModelType(model_spec: Type[T], required: bool=False, **kwargs) -> T:
    return _models.JsonModelType(model_spec, required=required, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def JsonSchemalessDictType(default: dict=_Undefined, required: bool=False, **kwargs) -> dict:
    return _models.JsonSchemalessDictType(default=default, required=required, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def LongType(default: int=_Undefined, min_value: int=None, max_value: int=None, required: bool=False, **kwargs) -> int:
    return _models.LongType(default=default, min_value=min_value, max_value=max_value, required=required, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def ListType(field: T, default: List[T]=_Undefined, min_size: int=None, max_size: int=None, required: bool=False, **kwargs) -> List[T]:
    return _models.ListType(field, default=default, min_size=min_size, max_size=max_size, required=required, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def ModelType(model_spec: Type[T], required: bool=False, **kwargs) -> T:
    return _models.ModelType(model_spec, required=required, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def SchemalessDictType(default: dict=_Undefined, required: bool=False, **kwargs) -> dict:
    return _models.SchemalessDictType(default=default, required=required, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def StringType(default: str=_Undefined, choices: List[str]=None, regex: str=None, required: bool=False, **kwargs) -> str:
    return _models.StringType(default=default, choices=choices, regex=regex, required=required, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def UUIDType(required: bool=False, **kwargs) -> str:
    return _models.UUIDType(required=required, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def IPAddressType(valid_values: Iterable = None, version: int = None, **kwargs) -> Union[_models.IPAddressType.native_types]:
    return _models.IPAddressType(valid_values=valid_values, version=version, **kwargs)


# noinspection PyTypeChecker,PyPep8Naming
def IPNetworkType(valid_values: Iterable = None, version: int = None, **kwargs) -> Union[_models.IPAddressType.native_types]:
    return _models.IPNetworkType(valid_values=valid_values, version=version, **kwargs)
