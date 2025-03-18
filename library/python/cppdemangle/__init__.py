from . import _demangle


class Error(Exception):
    pass


class UnknownError(Error):
    pass


class InvalidArgs(Error):
    pass


class InvalidMangledName(Error):
    pass


class MemoryAllocFailure(Error):
    pass


_errors = {
    -4: UnknownError,
    -3: InvalidArgs,
    -2: InvalidMangledName,
    -1: MemoryAllocFailure,
}


def _check(result):
    if isinstance(result, int):
        raise _errors[result]()
    return result


def itanium(mangled_name):
    return _check(_demangle.itanium(mangled_name))


def microsoft(mangled_name):
    return _check(_demangle.microsoft(mangled_name))


def cppdemangle(mangled_name):
    if mangled_name.startswith('_Z'):
        return itanium(mangled_name)
    if mangled_name.startswith('__Z'):
        return itanium(mangled_name[1:])
    if mangled_name.startswith('?'):
        return microsoft(mangled_name)
    return mangled_name
