"""Miscellaneous utils."""

import functools
import ipaddress
import itertools
import os
import random
import threading
import time
import uuid

from concurrent.futures import ThreadPoolExecutor

from yc_common import constants
from yc_common.context import get_context, context_session, apply_context
from yc_common.exceptions import Error, LogicalError


class Thread(threading.Thread):
    def __init__(self, derive_context, *args, context=None, **kwargs):
        super().__init__(*args, **kwargs)

        self.__context = get_context().to_dict() if derive_context else {}
        if context is not None:
            self.__context.update(context)

    def run(self):
        with context_session(**self.__context):
            super().run()


def drop_none(d, none=None):
    return {k: v for k, v in d.items() if v is not none}


def filter_dict(d, fields, allow):
    return {
        field: value for field, value in d.items() if (
            allow and field in fields or
            not allow and field not in fields
        )
    }


def min_or_none(*values):
    return min((value for value in values if value is not None), default=None)


def doesnt_override(klass):
    """A decorator which ensures that a class doesn't override anything from its base classes."""

    def decorator(method):
        if hasattr(klass, method.__name__):
            raise Error("{} overrides {}.{}.", method.__name__, klass.__name__, method.__name__)
        return method

    return decorator


def not_implemented(method):
    @functools.wraps(method)
    def decorator(*args, **kwargs):
        raise NotImplementedError(method.__name__ + "() method is not implemented.")

    return decorator


def generate_id():
    id = str(uuid.uuid4())

    # Always generate IDs with leading numeric character to be able reliably distinguish them from resource names
    if id[:1] not in "0123456789":
        id = str(random.randint(0, 9)) + id[1:]

    return id


def is_resource_id(name):
    return name[:1] in "0123456789"


def ellipsis_string(string, max_length, ellipsis=">...<"):
    if len(string) <= max_length:
        return string

    length = (max_length - len(ellipsis)) // 2
    if length < 1:
        return string

    return string[:length] + ellipsis + string[-length:]


def check_number_javascript_compatibility(number):
    # All numbers are stored in JavaScript as double values, so if number is very big (> 15 decimal digits) it will be
    # rounded.

    if float(number) != number:
        raise Exception("The number is not compatible with JavaScript.")

    return number


def format_call(callable, args, kwargs):
    name = callable.__name__ if hasattr(callable, "__name__") else callable.__class__.__name__
    return name + "(" + ", ".join(itertools.chain(
        (_format_function_arg(arg) for arg in args),
        (str(key) + "=" + _format_function_arg(value) for key, value in kwargs.items())
    )) + ")"


def _format_function_arg(arg):
    if isinstance(arg, str):
        return ellipsis_string(repr(arg), 100)
    elif arg is None or isinstance(arg, int):
        return repr(arg)
    else:
        return "obj"


def timestamp():
    return int(time.time())


def timestamp_ms():
    return int(time.time() * constants.SECOND_MILLISECONDS)


def race_condition_iterator():
    tries = 10

    while tries > 0:
        tries -= 1
        yield tries == 0


def safe_zip(*iterables):
    fill_value = object()

    for chain in itertools.zip_longest(*iterables, fillvalue=fill_value):
        for value in chain:
            if value is fill_value:
                raise LogicalError("Iterable lengths don't match.")

        yield chain


def parallelize_execution(callables, catch_exceptions=False, max_workers=None):
    futures = []

    with ThreadPoolExecutor(max_workers) as executor:
        for callable in callables:
            futures.append(executor.submit(apply_context(callable)))

    if catch_exceptions:
        results = []

        for future in futures:
            try:
                result = future.result()
            except Exception as e:
                result = e

            results.append(result)

        return results
    else:
        return [future.result() for future in futures]


def parallelize_calls(callable, args_list, **kwargs):
    return parallelize_execution((functools.partial(callable, *args) for args in args_list), **kwargs)


def parallelize_map(callable, args, **kwargs):
    return parallelize_calls(callable, ((arg,) for arg in args), **kwargs)


def join_thread(thread, timeout=None):
    if thread is not None and thread.is_alive():
        thread.join(timeout=timeout)


def configure_environment(binary_path):
    """
    setup.py automatically generates virtual environment activation code and inserts it into binaries enumerated in
    setup(scripts=...), but this code modifies process' PATH environment variable, which in most cases is unlikely if
    your code run other programs as subprocesses, because if these programs are written in Python they will be executed
    inside of the parent's virtual environment.

    This function fixes the environment of the current process.
    """

    venv_bin_path = os.path.dirname(os.path.realpath(binary_path))
    os.environ["PATH"] = os.pathsep.join(path for path in os.environ.get("PATH", "").split(os.pathsep)
                                         if os.path.normpath(path) != venv_bin_path)


def network_is_subnet_of(a: ipaddress._BaseNetwork, b: ipaddress._BaseNetwork):
    """
    Backport of subnet check from python3.7 ipaddress._BaseNetwork._is_subnet_of
    https://github.com/python/cpython/blob/v3.7.0/Lib/ipaddress.py#L976
    TODO: Switch to built-in method as production uses python>=3.7
    """
    try:
        # Always false if one is v4 and the other is v6.
        if a._version != b._version:
            raise TypeError("{a} and {b} are not of the same version".format(a=a, b=b))
        return (b.network_address <= a.network_address and
                b.broadcast_address >= a.broadcast_address)
    except AttributeError:
        raise TypeError("Unable to test subnet containment between {a} and {b}".format(a=a, b=b))


def timeout_iter(exc, timeout=10, retry_period=1):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        yield
        if deadline - time.monotonic() <= retry_period:
            break
        time.sleep(retry_period)
    raise exc
