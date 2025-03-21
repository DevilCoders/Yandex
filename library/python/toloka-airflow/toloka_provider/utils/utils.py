"""This module contains util functions."""
import json
import os
import pickle
from decimal import Decimal
from functools import wraps
from typing import Any, Type, TypeVar

from toloka.client import structure, add_headers

T = TypeVar('T')


def serialize_if_needed(func):

    @wraps(func)
    def wrapper(*args, **kwargs):
        default_xcom_backend = 'airflow.models.xcom.BaseXCom'
        xcom_backend = os.getenv('AIRFLOW__CORE__XCOM_BACKEND', default_xcom_backend)
        need_to_serialize = (xcom_backend == default_xcom_backend)
        result = add_headers('airflow')(func)(*args, **kwargs)
        if need_to_serialize and result is not None:
            if isinstance(result, list):
                result = list(map(lambda obj: obj.to_json(), result))
            else:
                result = result.to_json()
        return result

    return wrapper


def structure_from_conf(obj: Any, cl: Type[T]) -> T:
    if isinstance(obj, cl):
        return obj
    if isinstance(obj, bytes):
        try:
            return pickle.loads(obj)
        except Exception:
            pass
        obj = obj.decode()
    if isinstance(obj, str):
        obj = json.loads(obj, parse_float=Decimal)
    return structure(obj, cl)


def extract_id(obj: Any, cl: Type[T]) -> str:
    if isinstance(obj, str):
        try:
            obj = json.loads(obj, parse_float=Decimal)
        except Exception:
            return obj
        if isinstance(obj, int):
            return str(obj)
    return structure_from_conf(obj, cl).id
