import contextlib
import os
import shutil
import tempfile

from . import write


OPERATION_LABELS = {
    'unversioned': 'ADD',
    'modified': 'MODIFY',
    'missing': 'DELETE',
    'deleted': 'DELETE',
    'added': 'ADD',
    'replaced': 'REPLACE',
}


class ZipatchApplyError(Exception):
    pass


def no_op(*args):
    pass


def dump_zipatch(path, generate_func):
    z = write.ZipatchWriter()
    generate_func(z)
    z.save(path)


@contextlib.contextmanager
def dumped_zipatch(name, generate_func):
    dir_path = tempfile.mkdtemp()
    path = os.path.join(dir_path, '{}.zipatch'.format(name))
    try:
        dump_zipatch(path, generate_func)
        yield path
    finally:
        shutil.rmtree(dir_path, ignore_errors=True)


def zipatch_project_list(zipatch):
    dir_paths = frozenset(os.path.dirname(path) for path in zipatch.paths)
    return sorted(path for path in dir_paths if path)
