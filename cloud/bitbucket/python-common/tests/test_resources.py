"""Tests for yc_compute.resources module."""

import pytest

import yc_common
from yc_common import resources


@pytest.fixture
def yc_common_resources():
    return resources.package_resources(yc_common)


def test_open_rb(yc_common_resources):
    with yc_common_resources.open_rb("test/test_resource.txt") as f:
        assert f.read() == b"Tests use this file as packaged resource.\n"


def test_open_r(yc_common_resources):
    with yc_common_resources.open_r("test/test_resource.txt") as f:
        assert f.read() == "Tests use this file as packaged resource.\n"


def test_listdir(yc_common_resources):
    assert "test_resource.txt" in set(yc_common_resources.listdir("test"))


def test_isfile(yc_common_resources):
    assert yc_common_resources.isfile("test/test_resource.txt")
    assert not yc_common_resources.isfile("test")
    assert not yc_common_resources.isfile("unknown")
    assert not yc_common_resources.isfile("long/unknown")
    assert not yc_common_resources.isfile("test/unknown")
    assert not yc_common_resources.isfile("test/long/unknown")


def test_negative(yc_common_resources):
    with pytest.raises(NotADirectoryError):
        yc_common_resources.listdir("test/test_resource.txt")

    with pytest.raises(FileNotFoundError):
        yc_common_resources.listdir("test/unknown")

    with pytest.raises(IsADirectoryError):
        yc_common_resources.open_rb("test")

    with pytest.raises(FileNotFoundError):
        yc_common_resources.open_rb("test/unknown")

    with pytest.raises(IsADirectoryError):
        yc_common_resources.open_r("test")

    with pytest.raises(FileNotFoundError):
        yc_common_resources.open_r("test/unknown")
