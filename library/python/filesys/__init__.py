import contextlib
import tempfile
import shutil


def touch(filename):
    open(filename, 'a').close()


@contextlib.contextmanager
def temp_dir(suffix='', prefix='tmp', dir=None):
    """Context manager that creates temporary directory and removes on exit."""
    path = tempfile.mkdtemp(suffix=suffix, prefix=prefix, dir=dir)
    try:
        yield path
    finally:
        shutil.rmtree(path, True)
