# coding: utf-8
"""Math functions."""

# from decimal import Decimal
# from typing import TypeVar
# TNumber = TypeVar("TNumber", float, int, Decimal)


def clamp(value, minimum, maximum):
    # type: (TNumber, TNumber, TNumber) -> TNumber
    """Ограничить значение верхней и нижней границами."""
    return max(minimum, min(value, maximum))


def isclose(a, b, rel_tol=1e-09, abs_tol=0.0):
    # type: (float, float, float, float) -> bool
    """Функция для сравнения float'ов.

    Описание функции в стандартной библиотеке: `math.isclose`_.

    Бэкпорт из python3.5 для использования с python 2.7 (PEP 485).

    .. _math.isclose: https://docs.python.org/3/library/math.html#math.isclose
    """
    return abs(a - b) <= max(rel_tol * max(abs(a), abs(b)), abs_tol)
