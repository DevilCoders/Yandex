"""Module contains Arcadia-specific functions. """

try:
    import library.python.resource as resource
    _IS_ARCADIA = True
except ImportError:
    resource = None
    _IS_ARCADIA = False

from yc_common.arcadia.fs import ArcadiaFS


def is_arcadia() -> bool:
    global _IS_ARCADIA
    return _IS_ARCADIA


fs = ArcadiaFS() if is_arcadia() else None
