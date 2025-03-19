import inspect
import shutil
import tempfile

from unittest import mock

import pytest

if int(pytest.__version__.split(".")[0]) < 3:
    from _pytest.monkeypatch import monkeypatch as MonkeyPatch
else:
    from _pytest.monkeypatch import MonkeyPatch

import yc_common.logging
from yc_common import misc
from yc_common.misc import drop_none, generate_id

_UNSPECIFIED = object()


@pytest.fixture(autouse=True, scope="session")
def setup_logging(pytestconfig):
    yc_common.logging.setup(("yc", "tests", "functional_tests"), debug_mode=bool(pytestconfig.getoption("verbose")), devel_mode=True)


@pytest.fixture
def temp_dir(request):
    path = tempfile.mkdtemp()
    request.addfinalizer(lambda: shutil.rmtree(path))
    return path


@pytest.fixture
def monkeypatch_id(monkeypatch, request):
    return monkeypatch_function(monkeypatch, misc.generate_id, return_value=mock_id())


@pytest.fixture(scope="module")
def module_monkeypatch(request):
    monkeypatch = MonkeyPatch()
    request.addfinalizer(monkeypatch.undo)
    return monkeypatch


# Mocking tools

def mock_id(generate=False):
    if generate:
        return generate_id()
    else:
        return "00000000-0000-0000-0000-000000000000"


# Monkey patching tools

def patch(target, return_value=_UNSPECIFIED, side_effect=_UNSPECIFIED):
    return mock.patch(target, **_get_patch_kwargs(return_value, side_effect))


def patch_function(func, return_value=_UNSPECIFIED, side_effect=_UNSPECIFIED):
    return mock.patch.object(inspect.getmodule(func), func.__name__, **_get_patch_kwargs(return_value, side_effect))


def patch_method(method, return_value=_UNSPECIFIED, side_effect=_UNSPECIFIED):
    return mock.patch.object(_get_method_class(method), method.__name__, **_get_patch_kwargs(return_value, side_effect))


def monkeypatch_function(monkeypatch, func, return_value=_UNSPECIFIED, side_effect=_UNSPECIFIED):
    func_mock = mock.create_autospec(func, spec_set=True,
                                     **_get_patch_kwargs(return_value, side_effect, with_autospec=False))
    monkeypatch.setattr(inspect.getmodule(func), func.__name__, func_mock)
    return func_mock


def monkeypatch_method(monkeypatch, method, return_value=_UNSPECIFIED, side_effect=_UNSPECIFIED):
    method_mock = mock.create_autospec(method, spec_set=True,
                                       **_get_patch_kwargs(return_value, side_effect, with_autospec=False))
    monkeypatch.setattr(_get_method_class(method), method.__name__, method_mock)
    return method_mock


def _get_patch_kwargs(return_value, side_effect, with_autospec=True):
    kwargs = drop_none(dict(return_value=return_value, side_effect=side_effect), none=_UNSPECIFIED)
    if with_autospec:
        kwargs.update(autospec=True)
    return kwargs


def _get_method_class(method):
    # See explanation at http://stackoverflow.com/questions/25921450/given-a-method-how-do-i-return-the-class-it-belongs-to-in-python-3-3-onward/25922101#25922101
    class_name = method.__qualname__.split(".<locals>", 1)[0].rsplit(".", 1)[0]
    return getattr(inspect.getmodule(method), class_name)
