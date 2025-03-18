from functools import cache
from stubmaker.builder import override_module_import_path
from yatest.common import source_path


@cache  # ensures that function only runs once
def cached_override_module_import_path(module_root: str, arcadia_path: str):
    """
    Arcadia has its own misterious ways for detecting and importing modules
    due to its compile-python approach. This has strange side effects. For example
    `pkgutils.walk_packages` returns module duplicates such as
    `toloka.client.primitives.__init__` and `toloka.client.primitives`.
    So here we redefine arcadia's import mechanism and this seems to fix those problems
    """
    override_module_import_path(module_root, source_path(arcadia_path))
