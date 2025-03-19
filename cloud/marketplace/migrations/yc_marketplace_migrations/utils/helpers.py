import os
import time
from types import ModuleType
from typing import Iterable
from typing import List

try:
    import library.python.resource as resource

    IS_ARCADIA = True
    ARCADIA_PREFIX = "resfs/src/resfs/file/py/"
except ImportError:
    resource = None
    IS_ARCADIA = False
    ARCADIA_PREFIX = ""


def timestamp() -> int:
    return int(time.time())


def list_migrations_files(migrations_module: ModuleType) -> List[str]:
    if IS_ARCADIA:
        arcadia_path = os.path.dirname(ARCADIA_PREFIX + migrations_module.__file__) + "/"
        files = resource.iterkeys(prefix=arcadia_path, strip_prefix=True)
    else:
        files = os.listdir(os.path.dirname(migrations_module.__file__))

    return __extract_migrations_names(files)


def __extract_migrations_names(files: Iterable[str]) -> List[str]:
    migration_names = []

    for filename in files:
        name, ext = os.path.splitext(filename)
        if not name.startswith("_") and ext == ".py":
            migration_names.append(name)

    return migration_names
