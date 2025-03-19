import pathlib

from tools.synchronize.models import FileCollection


def test_file_collection_get_files(testdata):
    collection = FileCollection(directory="files")
    files = list(collection.get_files(pathlib.Path(".")))

    assert set(files) == {
        (pathlib.Path("first.txt"), pathlib.Path("files/first.txt")),
        (pathlib.Path("second.tf"), pathlib.Path("files/second.tf")),
    }


def test_file_collection_get_files_with_subdirectories(testdata):
    collection = FileCollection(directory="files", glob="**/*")
    files = list(collection.get_files(pathlib.Path(".")))

    assert len(files) == 4
    assert (pathlib.Path("subdirectory/subfile"), pathlib.Path("files/subdirectory/subfile")) in files


def test_file_collection_get_files_with_base_path(testdata):
    collection = FileCollection(directory="subdirectory")
    files = list(collection.get_files(pathlib.Path("files")))

    assert files == [(pathlib.Path("subfile"), pathlib.Path("files/subdirectory/subfile"))]


def test_file_collection_get_files_with_exclude(testdata):
    collections = FileCollection(directory="files/", exclude=["first.txt"])
    files = list(collections.get_files(pathlib.Path(".")))

    assert files == [(pathlib.Path("second.tf"), pathlib.Path("files/second.tf"))]


def test_file_collection_get_files_with_file_mask(testdata):
    collections = FileCollection(directory="files/", glob="**/*.txt")
    files = list(collections.get_files(pathlib.Path(".")))

    assert len(files) == 2
