
import os
import re
import pytest

from library.python import fs
from yatest import common as yac

# logger = logging.getLogger(__name__)

AUX_PROJECTS_ROOT = 'tools/mkdocs_builder/tests/data'
TEST_PROJECTS_ROOT = 'devtools/ymake/tests/docsbuild/data'
TOOL = yac.binary_path('tools/mkdocs_builder/mkdocs_builder')
TOOL_PY2 = yac.binary_path('tools/mkdocs_builder/tests/py2_bin/mkdocs_builder')


def _tool(python_version):
    return TOOL if python_version == 'py3' else TOOL_PY2


@pytest.mark.parametrize('v', ['py2', 'py3'])
def test_raw(v):
    _test_raw(v, 'raw')


@pytest.mark.parametrize('v', ['py2', 'py3'])
def test_raw_with_plugin(v):
    _test_raw(v, 'raw_with_plugin')


def _test_raw(v, project_name):
    output_dir = yac.work_path(project_name)
    yac.execute([_tool(v), '--docs-dir', os.path.join(yac.source_path(), AUX_PROJECTS_ROOT),
                 '--config', os.path.join(yac.source_path(), AUX_PROJECTS_ROOT, project_name + '.yml'),
                 '--output-dir', output_dir,
                 ])

    with open(os.path.join(output_dir, 'index.html'), 'r') as page:
        content = page.read()
    assert 'Hello world!!' in content
    assert '__Hello world!!__' not in content
    # in yfm20 mode boilerplate html code will be generated,
    # but the value under {% raw %} still will be visibly rendered as plain text
    assert '**' in content


@pytest.mark.parametrize('v', ['py2', 'py3'])
def test_preprocess(v):
    project_name = 'docs_vars'
    output_dir = yac.work_path(v + '/' + project_name)
    yac.execute([_tool(v), '--docs-dir', os.path.join(yac.source_path(), TEST_PROJECTS_ROOT, project_name),
                 '--config', os.path.join(yac.source_path(), TEST_PROJECTS_ROOT, project_name, 'mkdocs.yml'),
                 '--output-dir', output_dir,
                 '--var', 'version=1',
                 '--preprocess-md-only'])

    assert not os.path.exists(os.path.join(output_dir, 'index.html'))

    with open(os.path.join(output_dir, 'index.md'), 'r') as page:
        content = page.read()
    assert 'Brave new thought!' in content
    assert 'Old man drone...' not in content


@pytest.mark.parametrize('v', ['py2', 'py3'])
def test_dependency(v):
    dependency_name = 'docs_vars'
    dep_spec = ':'.join([yac.work_path(), TEST_PROJECTS_ROOT + '/' + dependency_name, 'preprocessed.tar.gz'])
    yac.execute([_tool(v), '--docs-dir', os.path.join(yac.source_path(), TEST_PROJECTS_ROOT, dependency_name),
                 '--config', os.path.join(yac.source_path(), TEST_PROJECTS_ROOT, dependency_name, 'mkdocs.yml'),
                 '--output-tar', dep_spec.replace(':', '/'),
                 '--var', 'version=1',
                 '--preprocess-md-only'])

    project_name = 'peerdir_from_docs'
    output_dir = yac.work_path(v + '/' + project_name)
    # this run expects preprocessed.tar.gz to be on fs
    yac.execute([_tool(v), '--docs-dir', os.path.join(yac.source_path(), TEST_PROJECTS_ROOT, project_name),
                 '--config', os.path.join(yac.source_path(), TEST_PROJECTS_ROOT, project_name, 'mkdocs.yml'),
                 '--output-dir', output_dir,
                 '--var', 'version=2',
                 '--dep', dep_spec])

    with open(os.path.join(output_dir, 'index.html'), 'r') as page:
        content = page.read()
    assert 'This is right' in content
    assert 'And this should not appear in result' not in content

    with open(os.path.join(output_dir, TEST_PROJECTS_ROOT, dependency_name, 'index.html'), 'r') as page:
        content = page.read()
    assert 'Brave new thought!' in content
    assert 'Old man drone...' not in content


@pytest.mark.parametrize('v', ['py2', 'py3'])
def test_include_source_file(v):
    include_name = 'devtools/dummy_arcadia/test_java_coverage/src/consoleecho/CrazyCalculator.java'
    include_name2 = 'devtools/dummy_arcadia/test_java_coverage/ya.make'

    project_name = 'include_sources'

    output_dir_new_scheme = yac.work_path(v + '/' + project_name + '_new_scheme')
    fs.copy_tree(yac.source_path(os.path.dirname(include_name2)),
                 os.path.join(yac.work_path(v + '/' + project_name + '_inc'), os.path.dirname(include_name2)))
    yac.execute([_tool(v), '--docs-dir', os.path.join(yac.source_path(), TEST_PROJECTS_ROOT, project_name),
                 '--config', os.path.join(yac.source_path(), TEST_PROJECTS_ROOT, project_name, 'config.yml'),
                 '--output-dir', output_dir_new_scheme,
                 '--sandbox-dir', yac.work_path(v + '/' + project_name + '_inc'),
                 ])

    with open(os.path.join(output_dir_new_scheme, 'index.html'), 'r') as page:
        content = re.sub('<.*?>', '', page.read())

    with open(yac.source_path(include_name), 'r') as page:
        include_content = ''.join(page.readlines()[:13])

    assert include_content in content

    with open(os.path.join(output_dir_new_scheme, 'nested', 'page', 'index.html'), 'r') as page:
        content_nested = re.sub('<.*?>', '', page.read())

    with open(yac.source_path(include_name2), 'r') as page:
        include_content_nested = ''.join(page.readlines()[0])

    assert include_content_nested in content_nested
