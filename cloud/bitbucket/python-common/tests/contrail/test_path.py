import pytest

from uuid import uuid4

from yc_common.clients.contrail.utils import Path
from yc_common.clients.contrail.exceptions import AbsPathRequiredError


def test_cd():
    p = Path("/")
    p = p / "foo" / "bar"
    assert str(p) == "/foo/bar"
    p = p / ".." / "foo"
    assert str(p) == "/foo/foo"


def test_resource():
    p = Path("/foo/{}".format(str(uuid4())))
    assert p.is_resource
    p = Path("/foo/bar")
    assert p.is_resource
    p = Path("/")
    assert not p.is_resource
    p = Path("/75963ada-2c70-4eeb-8daf-f24ce920ff7b")
    assert not p.is_resource
    p = Path("foo/bar")
    with pytest.raises(AbsPathRequiredError):
        p.is_resource


def test_collection():
    p = Path("/foo")
    assert p.is_collection
    p = Path("/foo/bar")
    assert not p.is_collection
    p = Path("/")
    assert p.is_collection
    p = Path("foo/bar")
    with pytest.raises(AbsPathRequiredError):
        p.is_collection


def test_uuid():
    p = Path("/foo/{}".format(str(uuid4())))
    assert p.is_uuid
    p = Path("/foo/bar")
    assert not p.is_uuid


def test_fq_name():
    p = Path("/foo/{}".format(str(uuid4())))
    assert not p.is_fq_name
    p = Path("/foo/bar")
    assert p.is_fq_name


def test_root():
    p = Path("/")
    assert p.is_root
    p = Path("/foo")
    assert not p.is_root


def test_base():
    p = Path("/foo")
    assert p.base == "foo"
    p = Path("/foo/{}".format(uuid4()))
    assert p.base == "foo"
    p = Path("/")
    assert p.base == ""


def test_unicode():
    p = Path("/éà")
    assert p.base == "??"
