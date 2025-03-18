import pytest


from _import_override import cached_override_module_import_path
from library.python.toloka_client.src.tk_stubgen.builder.tk_representations_tree_builder import TolokaKitRepresentationTreeBuilder
from library.python.toloka_client.src.tk_stubgen.constants.stubs import skip_modules, broken_modules, type_ignored_modules
from library.python.toloka_client.src.tk_stubgen.viewers.stub_viewer import TolokaKitStubViewer
from stubmaker.builder import traverse_modules
from yatest.common import source_path


def parametrize(modules_info):
    for module_root, arcadia_sources_dir in modules_info:
        cached_override_module_import_path(module_root, arcadia_sources_dir)

    params = [
        pytest.param(module, id=module_name, marks=broken_modules.get(module.__name__, []))
        for module_root, arcadia_sources_dir in modules_info
        for module_name, module in traverse_modules(
            module_root=module_root,
            sources_path=source_path(arcadia_sources_dir),
            skip_modules=skip_modules,
        )
    ]

    return pytest.mark.parametrize('module', params)


@parametrize([
    ('toloka', 'library/python/toloka-kit/src'),
    ('crowdkit', 'library/python/crowd-kit/crowdkit'),
])
def test_stubs(module):

    # Creating module's representation
    builder = TolokaKitRepresentationTreeBuilder(module.__name__, module, type_ignored_modules=type_ignored_modules)

    # Creating module's stub
    module_view = TolokaKitStubViewer().view(builder.module_rep)

    # Checking that stub is not outdated
    with open(source_path(module.__file__) + 'i') as stub_flo:
        assert module_view == stub_flo.read()
