"""Tests for yc_common.arcadia."""

import pytest

from yc_common import arcadia


if not arcadia.is_arcadia():
    pytest.skip("skipping arcadia tests", allow_module_level=True)


def test_open_rb():
    with arcadia.fs.open_rb("arcadia/file.txt") as f:
        assert f.read() == b"This is a content.\n"


def test_open_r():
    with arcadia.fs.open_r("arcadia/file.txt") as f:
        assert f.read() == "This is a content.\n"


def test_listdir():
    assert set(arcadia.fs.listdir("arcadia")) == {"file.txt", "folder"}
    assert set(arcadia.fs.listdir("arcadia/folder")) == {"another_file.txt"}


def test_isfile():
    assert arcadia.fs.isfile("arcadia/file.txt")
    assert arcadia.fs.isfile("arcadia/folder/another_file.txt")
    assert not arcadia.fs.isfile("arcadia")
    assert not arcadia.fs.isfile("unknown")
    assert not arcadia.fs.isfile("long/unknown")
    assert not arcadia.fs.isfile("arcadia/long/unknown")

    folder = arcadia.fs.open_dir("arcadia/folder")
    assert folder.isfile("another_file.txt")
    assert not folder.isfile("unknown")
    assert not folder.isfile("long/unknown")


def test_subdir_operations():
    arcadia_folder = arcadia.fs.open_dir("arcadia")
    assert set(arcadia_folder.listdir("folder")) == {"another_file.txt"}

    folder = arcadia_folder.open_dir("folder")

    assert set(folder.items()) == {"another_file.txt"}

    with folder.open_rb("another_file.txt") as f:
        assert f.read() == b"This is an another content.\n"


def test_negative():
    with pytest.raises(FileNotFoundError):
        arcadia.fs.listdir("arcadia/unknown directory")

    with pytest.raises(NotADirectoryError):
        arcadia.fs.listdir("arcadia/file.txt")

    with pytest.raises(FileNotFoundError):
        arcadia.fs.open_dir("unknown directory")

    with pytest.raises(NotADirectoryError):
        arcadia.fs.open_dir("arcadia/file.txt")

    with pytest.raises(IsADirectoryError):
        arcadia.fs.open_rb("arcadia")

    with pytest.raises(FileNotFoundError):
        arcadia.fs.open_rb("arcadia/unknown_file.txt")

    with pytest.raises(IsADirectoryError):
        arcadia.fs.open_r("arcadia")

    with pytest.raises(FileNotFoundError):
        arcadia.fs.open_r("arcadia/unknown_file.txt")
