import os
import pytest
import re

from _import_override import cached_override_module_import_path
from library.python.toloka_client.src.tk_stubgen.builder.tk_representations_tree_builder import TolokaKitRepresentationTreeBuilder
from library.python.toloka_client.src.tk_stubgen.constants.markdowns import skip_modules, broken_modules
from library.python.toloka_client.src.tk_stubgen.viewers.markdown_viewer import TolokaKitMarkdownViewer
from library.python.toloka_client.src.tk_stubgen.util import GitHubSourceFinder
from stubmaker.builder import traverse_modules
from yatest.common import source_path


def parametrize(modules_info):
    for module_root, arcadia_sources_dir, _, _ in modules_info:
        cached_override_module_import_path(module_root, arcadia_sources_dir)

    params = [
        pytest.param(
            module,
            module_root,
            source_path(arcadia_reference_dir),
            github_source_url,
            id=module_name,
            marks=broken_modules.get(module.__name__, []),
        )
        for module_root, arcadia_sources_dir, arcadia_reference_dir, github_source_url in modules_info
        for module_name, module in traverse_modules(
            module_root=module_root,
            sources_path=source_path(arcadia_sources_dir),
            skip_modules=skip_modules,
        )
    ]

    return pytest.mark.parametrize(('module', 'module_root', 'reference_dir', 'github_source_url'), params)


def _strip_source_code_line_number(line):
    def replace(match):
        if not match['line_number']:
            return match.group()
        start_pos = match.start('line_number') - match.start()
        end_pos = match.end('line_number') - match.start()
        before = match.group()[:start_pos]
        after = match.group()[end_pos:]
        return before + after

    pattern = re.compile(r'(?P<prefix>\[Source code\]\(http[^#]*)(?P<line_number>[^)]*)')
    return pattern.sub(replace, line)


# TODO: findsource seems to return inconsistent line number in Arcadia. Skip source link check in markdowns for now.
def assert_markdowns_equal(left, right, message):
    left = left.split('\n')
    right = right.split('\n')
    assert len(left) == len(right), message
    for left_line, right_line in zip(left, right):
        left_line = _strip_source_code_line_number(left_line)
        right_line = _strip_source_code_line_number(right_line)
        assert left_line == right_line, message


@parametrize([
    ('toloka', 'library/python/toloka-kit/src', 'library/python/toloka-kit/docs/reference',
     'https://github.com/Toloka/toloka-kit/blob/v0.1.25/src'),
    ('crowdkit', 'library/python/crowd-kit/crowdkit', 'library/python/crowd-kit/docs/reference',
     'https://github.com/Toloka/crowd-kit/blob/v1.0.0/crowdkit'),
])
def test_markdowns(module, module_root, reference_dir, github_source_url):

    # Creating module's representation
    builder = TolokaKitRepresentationTreeBuilder(
        module.__name__, module, module_root=module_root, preserve_forward_references=False,
    )

    # Creating module's stub
    viewer = TolokaKitMarkdownViewer(source_link_finder=GitHubSourceFinder(github_source_url, handle_unknown_source='ignore'))

    # Checking that stub is not outdated
    for name, markdown in viewer.get_markdown_files_for_module(builder.module_rep):
        filename = os.path.join(reference_dir, f'{name}.md')
        with open(filename) as stub_flo:
            assert_markdowns_equal(markdown, stub_flo.read(), filename)
